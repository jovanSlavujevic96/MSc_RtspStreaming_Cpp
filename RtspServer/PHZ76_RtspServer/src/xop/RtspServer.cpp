#include "RtspServer.h"
#include "RtspConnection.h"

#include "net/SocketUtil.h"
#include "net/Logger.h"

using namespace xop;

RtspServer::RtspServer(net::EventLoop* loop)
	: net::TcpServer(loop)
{

}

RtspServer::~RtspServer()
{
	
}

std::shared_ptr<RtspServer> RtspServer::Create(net::EventLoop* loop)
{
	std::shared_ptr<RtspServer> server(new RtspServer(loop));
	return server;
}

MediaSessionId RtspServer::AddSession(MediaSession* session)
{
    return RtspServer::AddSession(std::shared_ptr<MediaSession>(session));
}

MediaSessionId RtspServer::AddSession(std::shared_ptr<MediaSession> session)
{
    std::lock_guard<std::mutex> locker(mutex_);

    if (rtsp_suffix_map_.find(session->GetRtspUrlSuffix()) != rtsp_suffix_map_.end())
    {
        return 0;
    }

    MediaSessionId sessionId = session->GetMediaSessionId();
    rtsp_suffix_map_.emplace(session->GetRtspUrlSuffix(), sessionId);
    media_sessions_.emplace(sessionId, session);

    return sessionId;
}

void RtspServer::RemoveSession(MediaSessionId sessionId)
{
    std::lock_guard<std::mutex> locker(mutex_);

    auto iter = media_sessions_.find(sessionId);
    if(iter != media_sessions_.end()) {
        rtsp_suffix_map_.erase(iter->second->GetRtspUrlSuffix());
        media_sessions_.erase(sessionId);
    }
}

MediaSession::Ptr RtspServer::LookMediaSession(const std::string& suffix)
{
    std::lock_guard<std::mutex> locker(mutex_);

    auto iter = rtsp_suffix_map_.find(suffix);
    if(iter != rtsp_suffix_map_.end()) 
    {
        return media_sessions_[iter->second];
    }

    return nullptr;
}

MediaSession::Ptr RtspServer::LookMediaSession(MediaSessionId session_Id)
{
    std::lock_guard<std::mutex> locker(mutex_);

    auto iter = media_sessions_.find(session_Id);
    if(iter != media_sessions_.end()) {
        return iter->second;
    }

    return nullptr;
}

bool RtspServer::PushFrame(MediaSessionId session_id, MediaChannelId channel_id, AVFrame& frame)
{
    static std::unordered_map<MediaSessionId, MediaSession::Ptr>::iterator iter;

    std::lock_guard<std::mutex> locker(mutex_);
    iter = media_sessions_.find(session_id);
    if (iter != media_sessions_.end() && iter->second->GetNumClient() != 0)
    {
        return iter->second->HandleFrame(channel_id, frame);
    }

    return false;
}

net::TcpConnection::Ptr RtspServer::OnConnect(SOCKET sockfd)
{	
	return std::make_shared<RtspConnection>(shared_from_this(), event_loop_->GetTaskScheduler().get(), sockfd);
}

