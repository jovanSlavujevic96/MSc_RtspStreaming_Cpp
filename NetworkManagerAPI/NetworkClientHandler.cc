#include <sstream>
#include <cstring>

#include "NetworkClientHandler.h"
#include "StringPackage.h"

#define MESSAGE_LEN 1024u

#define NUMBER_OF_STATUS_STRINGS 2u //(4-2)

enum class NetworkClientHandler::eNetworkClientStatus
{
	REGISTER_USER = 0,
	LOGIN_USER,
	ERROR_MESSAGE,
	NONE
};

constexpr const char* const cNetworkClientStatusStrings[NUMBER_OF_STATUS_STRINGS] =
{
	"REGISTER_USER",
	"LOGIN_USER",
};

NetworkClientHandler::NetworkClientHandler(SOCKET sock_fd, std::unique_ptr<sockaddr_in> sock_addr) :
	IWorkerSocket{sock_fd, std::move(sock_addr)},
	mClientStatus{ eNetworkClientStatus::NONE }
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

void NetworkClientHandler::threadEntry()
{
	NetworkClientHandler& this_ = *this;
	StringPackage rcvPkg(MESSAGE_LEN);
	std::string sndPkg;
	while (true)
	{
		try
		{
			this_ >> &rcvPkg;
		}
		catch (...)
		{
			break;
		}
		NetworkClientHandler::parseMessage(rcvPkg.cData());
		NetworkClientHandler::formMessage(sndPkg);
		try
		{
			this_ << sndPkg;
		}
		catch (...)
		{
			break;
		}
	}
}

void NetworkClientHandler::parseMessage(const char* msg)
{
	for (uint8_t i = 0; i < NUMBER_OF_STATUS_STRINGS; ++i)
	{
		if (!strcmp(msg, cNetworkClientStatusStrings[i]))
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
			break;
		}
		case eNetworkClientStatus::LOGIN_USER:
		{
			break;
		}
		default:
			break;
	}
}

void NetworkClientHandler::formMessage(std::string& msg)
{

}
