#pragma once

#include <mutex>
#include <map>

#include "rtsp.h"

namespace rtsp
{

class RtspConnection;

class RtspPusher : public Rtsp
{
public:
	static std::shared_ptr<RtspPusher> Create(net::EventLoop* loop);
	~RtspPusher();

	void AddSession(MediaSession* session);
	void RemoveSession(MediaSessionId session_id);

	int  OpenUrl(std::string url, int msec = 3000);
	void Close();
	bool IsConnected();

	bool PushFrame(MediaChannelId channelId, AVFrame& frame);

private:
	friend class RtspConnection;

	RtspPusher(net::EventLoop *event_loop);
	MediaSession::Ptr LookMediaSession(MediaSessionId session_id);

	net::EventLoop* event_loop_ = nullptr;
	net::TaskScheduler* task_scheduler_ = nullptr;
	std::mutex mutex_;
	std::shared_ptr<RtspConnection> rtsp_conn_;
	std::shared_ptr<MediaSession> media_session_;
};

}
