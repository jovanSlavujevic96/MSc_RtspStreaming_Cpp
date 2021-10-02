#pragma once

#include <vector>
#include <string>
#include <mutex>

#include "ithread.h"
#include "ctcp_server.h"

struct OnDemandStreamInfo;
struct LiveStreamInfo;

class NetworkManager : 
	public CTcpServer, 
	public IThread
{
public:
	NetworkManager(uint16_t port) noexcept;
	~NetworkManager();

	void start() noexcept(false);
	void stop() noexcept;

	void updateLiveStream(const std::string& stream, const std::string& url);
	void updateOnDemandStream(const std::string& stream, const std::string& url, std::time_t timestamp);
private:
	void threadEntry() override final;

	void updateStreamMessage();
	void sendStreamMessage();

	std::vector<LiveStreamInfo*> mLiveStreams;
	std::vector<OnDemandStreamInfo*> mOnDemandStreams;
	std::string mStreamsInfoMessage;
	std::recursive_mutex mStreamsInfoMutex;
};
