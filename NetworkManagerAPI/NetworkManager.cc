#include <stdexcept>

#include "csocket_exception.h"

#include "NetworkClientHandler.h"
#include "NetworkManager.h"

NetworkManager::NetworkManager(uint16_t port, size_t num_of_streams) noexcept :
	CTcpServer{port},
	mPackage{ num_of_streams }
{
	CTcpServer::mAllocSocketFunction = [this](SOCKET fd, std::unique_ptr<sockaddr_in> addr) -> std::unique_ptr<CSocket>
		{ 
			return std::make_unique<NetworkClientHandler>(fd, std::move(addr), mPackage); 
		};
}

NetworkManager::~NetworkManager()
{
	NetworkManager::stop();
}

void NetworkManager::start() noexcept(false)
{
	if (!CSocket::mRunningSockAddr)
	{
		throw std::runtime_error("NetworkManager::start : Cannot start thread before initServer call.");
	}
	IThread::start();
}

void NetworkManager::stop() noexcept
{
	NetworkClientHandler* network_client = NULL;
	for (std::unique_ptr<CSocket>& client : CTcpServer::mClientSockets)
	{
		network_client = dynamic_cast<NetworkClientHandler*>(client.get());
		if (network_client)
		{
			network_client->stop();
		}
	}
	CSocket::closeSocket();
	IThread::join();
}

void NetworkManager::appendStream(const std::string& stream)
{
	mPackage.appendStream(stream.c_str());
}

void NetworkManager::threadEntry()
{
	NetworkClientHandler* network_client = NULL;
	size_t id;
	while (true)
	{
		try
		{
			id = CTcpServer::acceptClient();
			network_client = dynamic_cast<NetworkClientHandler*>(mClientSockets.at(id).get());
			if (network_client)
			{
				network_client->start();
			}
		}
		catch (const CSocketException&)
		{
			break;
		}
	}
}
