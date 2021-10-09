#include <sstream>

#include "NetworkEncryptor.h"
#include "NetworkDecryptor.h"
#include "NetworkUser.h"

#define MAX_MESSAGE_LEN 2048u //2kb

#define CMD_PREFFIX_WORD "CMD:"
#define CMD_STREAM_LIST "CMD:STREAM_LIST\r\n"

#define RESP_PREFFIX_WORD "RESP:"
#define RESP_OK "RESP:OK\r\n"
#define RESP_ERR "RESP:ERR\r\n"

static const size_t cCmdStreamListLen = std::strlen(CMD_STREAM_LIST);

static inline void SetStreamsAndUrls(const char* str, std::vector<std::string>& streams, std::vector<std::string>& urls) noexcept(false)
{
	const char* iterator = str;
	const char* end = std::strstr(str, "\r\n\r\n");
	if (!end)
	{
		throw std::runtime_error("Bad message format. Missing \"\\r\\n\\r\\n\".");
	}
	streams.clear();
	urls.clear();
	while (iterator != end)
	{
		if (iterator != str)
		{
			iterator += 2;
		}
		const char* tmp = std::strstr(iterator, "\r\n");
		if (!tmp)
		{
			throw std::runtime_error("Impossible use-case.");
		}
		if (streams.size() == urls.size())
		{
			streams.push_back(std::string(iterator, tmp));
		}
		else
		{
			urls.push_back(std::string(iterator, tmp));
		}
		iterator = tmp;
	}
}

NetworkUser::NetworkUser(const char* manager_ip, uint16_t manager_port) :
	CTcpClient{manager_ip, manager_port},
	mRcvPkg{ MAX_MESSAGE_LEN },
	mGotStreams{ false },
	mEncryptor{ new NetworkEncryptor },
	mDecryptor{ new NetworkDecryptor }
{

}

void NetworkUser::sendRequest(const std::string& message) noexcept(false)
{
	std::string output;
	mEncryptor->encryptText(message, output);
	*this << output;
}

void NetworkUser::getStreamsReqest() noexcept(false)
{
	const std::string get_streams_message = "GET_STREAMS\r\n\r\n";
	NetworkUser::sendRequest(get_streams_message);
}

void NetworkUser::signUpRequest(const std::string& username, const std::string& email, const std::string& password) noexcept(false)
{
	std::string sending_message;
	{
		std::stringstream signup_user_stream;
		signup_user_stream << "REGISTER_USER\r\n" << "USERNAME=" << username << "\r\n" << "EMAIL=" << email << "\r\n" << "PASSWORD=" << password << "\r\n\r\n";
		sending_message = signup_user_stream.str();
	}
	NetworkUser::sendRequest(sending_message);
}

void NetworkUser::signInRequest(const std::string& username_email, const std::string& password) noexcept(false)
{
	std::string sending_message;
	{
		std::stringstream signin_user_stream;
		signin_user_stream << "LOGIN_USER\r\n" << "USERNAME=" << username_email << "\r\n" << "PASSWORD=" << password << "\r\n\r\n";
		sending_message = signin_user_stream.str();
	}
	NetworkUser::sendRequest(sending_message);
}

void NetworkUser::receiveMessage() noexcept(false)
{
	std::string decryptedMessage;
	const char* ptr = NULL;
	int32_t ret = *this >> &mRcvPkg;

	mDecryptor->decryptText(mRcvPkg.cData(), ret, decryptedMessage);
	ptr = decryptedMessage.c_str();

	if (std::strstr(ptr, CMD_PREFFIX_WORD))
	{
		if (std::strstr(ptr, CMD_STREAM_LIST))
		{
			const char*  iterator = ptr + cCmdStreamListLen;
			::SetStreamsAndUrls(iterator, mStreams, mUrls);
			mGotStreams = true;
		}
	}
	else if (std::strstr(ptr, RESP_PREFFIX_WORD))
	{
		if (!std::strstr(ptr, RESP_OK))
		{
			throw std::runtime_error("Bad message format. RESP message is not OK.");
		}
	}
	else
	{
		throw std::runtime_error("Bad message format. Message is nor CMD nor RESP.");
	}
}

const std::vector<std::string>& NetworkUser::getStreams() const
{
	return mStreams;
}

const std::vector<std::string>& NetworkUser::getUrls() const
{
	return mUrls;
}

bool NetworkUser::gotStreams() const
{
	return mGotStreams;
}

void NetworkUser::resetStreams()
{
	mGotStreams = false;
}
