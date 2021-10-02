#pragma once

#include <memory>
#include <thread>
#include <mutex>
#include <vector>

#include "xop/MediaSession.h"

struct OnDemandHandler
{
	std::shared_ptr<xop::RtpConnection> connection;
	bool running;
	std::thread thread;
	std::string reading_video;
};

class OnDemandSession : public xop::MediaSession
{
public:
	OnDemandSession(const std::string& url);
	~OnDemandSession();

	void setReadingVideo(const std::string& video);
	const std::string& getReadingvideo() const;
	size_t getActiveClients();
	size_t getClientsReadingVideo(const std::string& video);
	bool AddSource(xop::MediaChannelId channel_id, xop::MediaSource* source) override;
	bool StartMulticast() override;
	void onDemandLoop(OnDemandHandler* handler);
private:
	std::vector<OnDemandHandler*> mHandler;
	std::mutex mRunningMutex;
	std::mutex mReadingMutex;
	std::string mReadingVideo;
	bool mRunning;
};
