#include <stdexcept>
#include <algorithm>

#include "csocket_exception.h"

#include "NetworkClientHandler.h"
#include "NetworkManager.h"

struct StreamInfo
{
	std::string stream;
	std::string url;
	bool is_live;
	bool is_busy;
};

inline constexpr bool sortStreamInfo(const StreamInfo* left, const StreamInfo* right)
{
	return left->is_live;
}

NetworkManager::NetworkManager(uint16_t port) noexcept :
	CTcpServer{port}
{
	CTcpServer::mAllocSocketFunction = [this](SOCKET fd, std::unique_ptr<sockaddr_in> addr) -> std::unique_ptr<CSocket>
		{ 
			return std::make_unique<NetworkClientHandler>(fd, std::move(addr)); 
		};
}

NetworkManager::~NetworkManager()
{
	NetworkManager::stop();

	for (StreamInfo* stream_info : mStreamsInfoList)
	{
		delete stream_info;
	}
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

void NetworkManager::update(const std::string& stream, const std::string& url, bool is_live, bool is_busy, bool send)
{
	NetworkManager::update(stream.c_str(), url.c_str(), is_live, is_busy, send);
}

void NetworkManager::update(const char* stream, const char* url, bool is_live, bool is_busy, bool send)
{
	StreamInfo* tmp = NULL;
	mStreamsInfoMutex.lock();
	for (StreamInfo* stream_info : mStreamsInfoList)
	{
		if (stream_info->stream == stream)
		{
			tmp = stream_info;
			break;
		}
	}
	if (!tmp)
	{
		tmp = new StreamInfo;
		tmp->stream = stream;
		tmp->url = url;
		mStreamsInfoList.push_back(tmp);
	}
	tmp->is_live = is_live;
	tmp->is_busy = is_busy;

	std::sort(mStreamsInfoList.begin(), mStreamsInfoList.end(), ::sortStreamInfo);

	mStreamsInfoMessage = "CMD:STREAM_LIST\r\n";
	for (StreamInfo* stream_info : mStreamsInfoList)
	{
		if (stream_info->is_busy)
		{
			continue;
		}
		if (stream_info->is_live)
		{
			mStreamsInfoMessage += "LIVE:";
		}
		mStreamsInfoMessage += stream_info->stream + "\r\n";
		mStreamsInfoMessage += stream_info->url + "\r\n";
	}
	mStreamsInfoMessage += "\r\n";

	if (!send)
	{
		goto exit;
	}
	for (const std::unique_ptr<CSocket>& sender : CTcpServer::mClientSockets)
	{
		try
		{
			*sender.get() << mStreamsInfoMessage;
		}
		catch (...)
		{
			continue;
		}
	}
exit:
	mStreamsInfoMutex.unlock();
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

				std::lock_guard<std::recursive_mutex> guard(mStreamsInfoMutex);
				*network_client << mStreamsInfoMessage;
			}
		}
		catch (const CSocketException&)
		{
			break;
		}
	}
}
