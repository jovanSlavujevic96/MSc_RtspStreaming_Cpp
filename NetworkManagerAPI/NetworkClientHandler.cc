#include <sstream>
#include <cstring>

#include "StringPackage.h"
#include "NetworkManager.h"
#include "NetworkEncryptor.h"
#include "NetworkDecryptor.h"
#include "NetworkClientHandler.h"

#define USERNAME_LINE "USERNAME="
#define USERNAME_LINE_LEN 9u

#define PASSWORD_LINE "PASSWORD="
#define PASSWORD_LINE_LEN 9u

#define EMAIL_LINE "EMAIL="
#define EMAIL_LINE_LEN 6u

#define REPLY_OK  "RESP:OK\r\n"
#define REPLY_ERR "RESP:ERR\r\n"

#define MESSAGE_LEN 1024u
#define NUMBER_OF_STATUS_STRINGS 3u //(5-2)

enum class NetworkClientHandler::eNetworkClientStatus
{
	REGISTER_USER = 0,
	LOGIN_USER,
	GET_STREAMS,
	ERROR_MESSAGE,
	NONE
};

constexpr const char* const cNetworkClientStatusStrings[NUMBER_OF_STATUS_STRINGS] =
{
	"REGISTER_USER",
	"LOGIN_USER",
	"GET_STREAMS"
};

NetworkClientHandler::NetworkClientHandler(SOCKET sock_fd, std::unique_ptr<sockaddr_in> sock_addr, std::shared_ptr<NetworkManager> manager) :
	IWorkerSocket{sock_fd, std::move(sock_addr)},
	mClientStatus{ eNetworkClientStatus::NONE },
	mManager{ manager },
	mGotAccess{ false },
	mEncryptor{ new NetworkEncryptor },
	mDecryptor{ new NetworkDecryptor }
{

}

NetworkClientHandler::~NetworkClientHandler()
{

}

void NetworkClientHandler::start() noexcept(false)
{
	if (INVALID_SOCKET == CSocket::mSocketFd || nullptr == CSocket::mTargetSockAddr)
	{
		std::stringstream ss;
		ss << "NetworkClientHandler::start :\n";
		ss << "\tINVALID Socket FD          = " << std::boolalpha << (INVALID_SOCKET == CSocket::mSocketFd) << std::endl;
		ss << "\tNULL Socket Target Address = " << std::boolalpha << (nullptr == CSocket::mTargetSockAddr) << std::endl;
		throw std::runtime_error(ss.str());
	}
	IThread::start();
}

void NetworkClientHandler::stop() noexcept
{
	CSocket::closeSocket();
	IThread::detach();
}

bool NetworkClientHandler::gotAccess() const
{
	return mGotAccess;
}

void NetworkClientHandler::sendMessage(const std::string& message) noexcept(false)
{
	std::string encrypted_message;
	mEncryptor->encryptText(message, encrypted_message);
	*this << encrypted_message;
}

void NetworkClientHandler::threadEntry()
{
	NetworkClientHandler& this_ = *this;
	StringPackage rcvPkg(MESSAGE_LEN);
	std::string sndPkg;
	std::string decryptedMessage;
	size_t ret;
	while (true)
	{
		decryptedMessage.clear();
		try
		{
			ret = this_ >> &rcvPkg;
			mDecryptor->decryptText(rcvPkg.cData(), ret, decryptedMessage);
		}
		catch (...)
		{
			break;
		}
		try
		{
			NetworkClientHandler::parseMessage(decryptedMessage.c_str());
			sndPkg = REPLY_OK;
			mGotAccess = true;
		}
		catch (const std::exception& e)
		{
			sndPkg = REPLY_ERR + std::string(e.what()) + "\r\n";
		}
		sndPkg += "\r\n";
		try
		{
			if (eNetworkClientStatus::GET_STREAMS == mClientStatus)
			{
				mManager->sendStreamMessage(this);
			}
			else
			{
				this->sendMessage(sndPkg);
			}
		}
		catch (...)
		{
			break;
		}
	}
}

void NetworkClientHandler::parseMessage(const char* msg) noexcept(false)
{
	const char* end_line_1 = std::strstr(msg, "\r\n");
	if (!end_line_1)
	{
		throw std::runtime_error("Bad message format.");
	}
	const std::string line_1(msg, end_line_1);
	for (uint8_t i = 0; i < NUMBER_OF_STATUS_STRINGS; ++i)
	{
		if (line_1 == cNetworkClientStatusStrings[i])
		{
			mClientStatus = static_cast<eNetworkClientStatus>(i);
			goto processing;
		}
	}
	mClientStatus = eNetworkClientStatus::ERROR_MESSAGE;
processing:
	switch (mClientStatus)
	{
		case eNetworkClientStatus::REGISTER_USER:
		{
			NetworkClientHandler::parseRegisterMessage(end_line_1);
			break;
		}
		case eNetworkClientStatus::LOGIN_USER:
		{
			NetworkClientHandler::parseLoginMessage(end_line_1);
			break;
		}
		default:
			break;
	}
}

void NetworkClientHandler::parseRegisterMessage(const char* end_line) noexcept(false)
{
	const char* end_line_2 = std::strstr(end_line + 2, "\r\n");
	const char* username_line_2 = std::strstr(end_line + 2, USERNAME_LINE);
	if (!end_line_2 || !username_line_2)
	{
		throw std::runtime_error("Bad message format.");
	}
	const std::string username(username_line_2 + USERNAME_LINE_LEN, end_line_2);
	bool ret;
	ret = mManager->checkExistanceSql(username.c_str(), "username");
	if (ret)
	{
		throw std::runtime_error(username + " already exists in database.");
	}
	const char* end_line_3 = std::strstr(end_line_2 + 2, "\r\n");
	const char* email_line_3 = std::strstr(end_line_2 + 2, EMAIL_LINE);
	if (!end_line_3 || !email_line_3)
	{
		throw std::runtime_error("Bad message format.");
	}
	const std::string email(email_line_3 + EMAIL_LINE_LEN, end_line_3);
	ret = mManager->checkExistanceSql(email.c_str(), "email");
	if (ret)
	{
		throw std::runtime_error(username + " already exists in database.");
	}
	const char* end_line_4 = std::strstr(end_line_3 + 2, "\r\n");
	const char* password_line_4 = std::strstr(end_line_3 + 2, PASSWORD_LINE);
	if (!end_line_4 || !password_line_4)
	{
		throw std::runtime_error("Bad message format.");
	}
	const std::string password(password_line_4 + PASSWORD_LINE_LEN, end_line_4);
	ret = mManager->insertNewUserSql(username, email, password);
	if (!ret)
	{
		throw std::runtime_error("Failed to insert new user in database.");
	}
}

void NetworkClientHandler::parseLoginMessage(const char* end_line)  noexcept(false)
{
	const char* end_line_2 = std::strstr(end_line + 2, "\r\n");
	const char* username_line_2 = std::strstr(end_line + 2, USERNAME_LINE);
	if (!end_line_2 || !username_line_2)
	{
		throw std::runtime_error("Bad message format.");
	}
	const std::string username(username_line_2 + USERNAME_LINE_LEN, end_line_2);
	bool ret;
	const char* type_of_data = std::strstr(username.c_str(), "@") ? "email" : "username";
	ret = mManager->checkExistanceSql(username.c_str(), type_of_data);
	if (!ret)
	{
		throw std::runtime_error("Failed to find " + username + " in database.");
	}
	const char* end_line_3 = std::strstr(end_line_2 + 2, "\r\n");
	const char* password_line_3 = std::strstr(end_line_2 + 2, PASSWORD_LINE);
	if (!end_line_3 || !password_line_3)
	{
		throw std::runtime_error("Bad message format.");
	}
	const std::string password(password_line_3 + PASSWORD_LINE_LEN, end_line_3);
	ret = mManager->checkPasswordSql(password.c_str(), username.c_str(), type_of_data);
	if (!ret)
	{
		throw std::runtime_error("Bad password.");
	}
}
