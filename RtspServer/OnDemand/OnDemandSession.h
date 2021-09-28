#pragma once

#include <memory>
#include <thread>
#include <mutex>
#include <vector>

#include "AvH264Writer.h"
#include "xop/MediaSession.h"

struct OnDemandHandler
{
	std::shared_ptr<xop::RtpConnection> connection;
	bool running;
	std::thread thread;
};

class OnDemandSession : public xop::MediaSession
{
public:
	OnDemandSession(const std::string& url);
	~OnDemandSession();

	bool AddSource(xop::MediaChannelId channel_id, xop::MediaSource* source) override;
	bool StartMulticast() override;
	void bindH264Writer(AvH264Writer* writer);
	void onDemandLoop(OnDemandHandler* handler);
private:
	AvH264Writer* mWriter;
	std::vector<OnDemandHandler*> mHandler;
	std::mutex mMutex;
	bool mRunning;
};
