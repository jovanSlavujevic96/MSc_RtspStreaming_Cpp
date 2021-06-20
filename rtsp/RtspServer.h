﻿#pragma once

#include <memory>
#include <string>
#include <mutex>
#include <unordered_map>

#include "net/TcpServer.h"
#include "rtsp.h"

namespace rtsp
{

class RtspConnection;

class RtspServer : public Rtsp, public net::TcpServer
{
public:    
	static std::shared_ptr<RtspServer> Create(net::EventLoop* loop);
	~RtspServer();

    MediaSessionId AddSession(MediaSession* session);
    void RemoveSession(MediaSessionId sessionId);

    bool PushFrame(MediaSessionId sessionId, MediaChannelId channelId, AVFrame& frame);

private:
    friend class RtspConnection;

	RtspServer(net::EventLoop* loop);
    MediaSession::Ptr LookMediaSession(const std::string& suffix);
    MediaSession::Ptr LookMediaSession(MediaSessionId session_id);
    virtual net::TcpConnection::Ptr OnConnect(SOCKET sockfd);

    std::mutex mutex_;
    std::unordered_map<MediaSessionId, std::shared_ptr<MediaSession>> media_sessions_;
    std::unordered_map<std::string, MediaSessionId> rtsp_suffix_map_;
};

}
