#include <stdexcept>
#include <algorithm>

#include "csocket_exception.h"

#include "NetworkClientHandler.h"
#include "NetworkManager.h"

struct LiveStreamInfo
{
	std::string stream;
	std::string url;
};

struct OnDemandStreamInfo
{
	std::string stream;
	std::string url;
	std::time_t timestamp;
};

inline constexpr bool sortOnDemandStreamInfo(const OnDemandStreamInfo* left, const OnDemandStreamInfo* right)
{
	return (left->timestamp < right->timestamp);
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

	for (OnDemandStreamInfo* stream_info : mOnDemandStreams)
	{
		delete stream_info;
	}

	for (LiveStreamInfo* stream_info : mLiveStreams)
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

void NetworkManager::updateLiveStream(const std::string& stream, const std::string& url)
{
	LiveStreamInfo* tmp = NULL;
	mStreamsInfoMutex.lock();
	for (LiveStreamInfo* stream_info : mLiveStreams)
	{
		if (stream_info->stream == stream)
		{
			tmp = stream_info;
			break;
		}
	}
	if (tmp)
	{
		goto unlock;
	}
	mLiveStreams.push_back(new LiveStreamInfo{ stream, url });

	NetworkManager::updateStreamMessage();
	NetworkManager::sendStreamMessage();
unlock:
	mStreamsInfoMutex.unlock();
}

void NetworkManager::updateOnDemandStream(const std::string& stream, const std::string& url, std::time_t timestamp)
{
	OnDemandStreamInfo* tmp = NULL;
	mStreamsInfoMutex.lock();
	for (OnDemandStreamInfo* stream_info : mOnDemandStreams)
	{
		if (stream_info->url == url)
		{
			tmp = stream_info;
			break;
		}
	}
	if (!tmp)
	{
		tmp = new OnDemandStreamInfo;
		tmp->url = url;
		mOnDemandStreams.push_back(tmp);
	}
	tmp->stream = stream;
	tmp->timestamp = timestamp;

	NetworkManager::updateStreamMessage();
	NetworkManager::sendStreamMessage();
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

void NetworkManager::updateStreamMessage()
{
	mStreamsInfoMessage = "CMD:STREAM_LIST\r\n";

	for (LiveStreamInfo* stream_info : mLiveStreams)
	{
		mStreamsInfoMessage += "LIVE:" + stream_info->stream + "\r\n";
		mStreamsInfoMessage += stream_info->url + "\r\n";
	}

	std::sort(mOnDemandStreams.begin(), mOnDemandStreams.end(), ::sortOnDemandStreamInfo);

	for (OnDemandStreamInfo* stream_info : mOnDemandStreams)
	{
		mStreamsInfoMessage += stream_info->stream + "\r\n";
		mStreamsInfoMessage += stream_info->url + "\r\n";
	}
	mStreamsInfoMessage += "\r\n";
}

void NetworkManager::sendStreamMessage()
{
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
}
