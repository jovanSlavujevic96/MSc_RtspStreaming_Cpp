﻿#include <cstring>
#include <ctime>
#include <map>
#include <forward_list>

#include "net/Logger.h"
#include "net/SocketUtil.h"
#include "net/MulticastAddr.h"

#include "MediaSession.h"
#include "RtpConnection.h"

using namespace xop;

std::atomic_uint MediaSession::last_session_id_(1);

MediaSession::MediaSession(const char* url_suffxx) :
	suffix_(url_suffxx) ,
	media_sources_(MAX_MEDIA_CHANNEL) ,
	buffer_(MAX_MEDIA_CHANNEL)
{
	has_new_client_ = false;
	session_id_ = ++last_session_id_;

	for(int n=0; n<MAX_MEDIA_CHANNEL; n++) {
		multicast_port_[n] = 0;
	}
}

MediaSession::MediaSession(const std::string& url_suffxx) :
	suffix_(url_suffxx),
	media_sources_(MAX_MEDIA_CHANNEL),
	buffer_(MAX_MEDIA_CHANNEL)
{
	has_new_client_ = false;
	session_id_ = ++last_session_id_;

	for (int n = 0; n < MAX_MEDIA_CHANNEL; n++) {
		multicast_port_[n] = 0;
	}
}

MediaSession* MediaSession::CreateNew(const std::string& url_suffix)
{
	return new MediaSession(url_suffix.c_str());
}

MediaSession* MediaSession::CreateNew(const char* url_suffix)
{
	return new MediaSession(url_suffix);
}

std::shared_ptr<MediaSession> MediaSession::CreateNewSmart(const std::string& url_suffix)
{
	return std::shared_ptr<MediaSession>(MediaSession::CreateNew(url_suffix));
}

std::shared_ptr<MediaSession> MediaSession::CreateNewSmart(const char* url_suffix)
{
	return std::shared_ptr<MediaSession>(MediaSession::CreateNew(url_suffix));
}

MediaSession::~MediaSession()
{
	if (multicast_ip_ != "") 
	{
		net::MulticastAddr::instance().Release(multicast_ip_);
	}
}

void MediaSession::AddNotifyConnectedCallback(const NotifyConnectedCallback& callback)
{
	notify_connected_callbacks_.push_back(callback);
}

void MediaSession::AddNotifyDisconnectedCallback(const NotifyDisconnectedCallback& callback)
{
	notify_disconnected_callbacks_.push_back(callback);
}

const std::string& MediaSession::GetRtspUrlSuffix() const
{
	return suffix_;
}

void MediaSession::SetRtspUrlSuffix(const std::string& suffix)
{
	suffix_ = suffix;
}

const std::string& MediaSession::GetRtspUrl() const
{
	return url_;
}

void MediaSession::SetRtspUrl(const std::string& suffix)
{
	url_ = suffix;
}

void MediaSession::SetRtspUrl(const char* suffix)
{
	url_ = suffix;
}

bool MediaSession::AddSource(MediaChannelId channel_id, MediaSource* source)
{
	source->SetSendFrameCallback(
		[this](MediaChannelId channel_id, const RtpPacket& pkt, std::shared_ptr<RtpConnection> connection)
		{
			static std::map<SOCKET, std::weak_ptr<xop::RtpConnection>>::iterator iter;
			std::lock_guard<std::recursive_mutex> lock(map_mutex_);
			int ret;
			(void)connection;
			for (iter = clients_.begin(); iter != clients_.end();)
			{
				auto conn = iter->second.lock();
				if (conn == nullptr)
				{
					clients_.erase(iter++);
				}
				else
				{
					if (conn->GetId() >= 0)
					{
						ret = conn->SendRtpPacket(channel_id, pkt);
						if (is_multicast_ && ret)
						{
							break;
						}
					}
					iter++;
				}
			}
			return true;
		});
	media_sources_[channel_id].reset(source);
	return true;
}

bool MediaSession::RemoveSource(MediaChannelId channel_id)
{
	media_sources_[channel_id] = nullptr;
	return true;
}

bool MediaSession::StartMulticast()
{  
	if (is_multicast_) {
		return true;
	}

	multicast_ip_ = net::MulticastAddr::instance().GetAddr();
	if (multicast_ip_ == "") {
		return false;
	}

	std::random_device rd;
	multicast_port_[channel_0] = htons(rd() & 0xfffe);
	multicast_port_[channel_1] = htons(rd() & 0xfffe);

	is_multicast_ = true;
	return true;
}

std::string MediaSession::GetSdpMessage(std::string ip, std::string session_name)
{
	if (sdp_ != "") {
		return sdp_;
	}
    
	if (media_sources_.empty()) {
		return "";
	}

	char buf[2048] = {0};

	snprintf(buf, sizeof(buf),
			"v=0\r\n"
			"o=- 9%ld 1 IN IP4 %s\r\n"
			"t=0 0\r\n"
			"a=control:*\r\n" ,
			(long)std::time(NULL), ip.c_str()); 

	if(session_name != "") {
		snprintf(buf+strlen(buf), sizeof(buf)-strlen(buf), 
				"s=%s\r\n",
				session_name.c_str());
	}
    
	if(is_multicast_) {
		snprintf(buf+strlen(buf), sizeof(buf)-strlen(buf),
				"a=type:broadcast\r\n"
				"a=rtcp-unicast: reflection\r\n");
	}
		
	for (uint32_t chn=0; chn<media_sources_.size(); chn++) {
		if(media_sources_[chn]) {	
			if(is_multicast_) {
				snprintf(buf+strlen(buf), sizeof(buf)-strlen(buf), 
						"%s\r\n",
						media_sources_[chn]->GetMediaDescription(multicast_port_[chn]).c_str()); 
                     
				snprintf(buf+strlen(buf), sizeof(buf)-strlen(buf), 
						"c=IN IP4 %s/255\r\n",
						multicast_ip_.c_str()); 
			}
			else {
				snprintf(buf+strlen(buf), sizeof(buf)-strlen(buf), 
						"%s\r\n",
						media_sources_[chn]->GetMediaDescription(0).c_str());
			}
            
			snprintf(buf+strlen(buf), sizeof(buf)-strlen(buf), 
					"%s\r\n",
					media_sources_[chn]->GetAttribute().c_str());
                     
			snprintf(buf+strlen(buf), sizeof(buf)-strlen(buf),											
					"a=control:track%d\r\n", chn);	
		}
	}
	sdp_ = buf;
	return sdp_;
}

MediaSource* MediaSession::GetMediaSource(MediaChannelId channel_id)
{
	if (media_sources_[channel_id]) {
		return media_sources_[channel_id].get();
	}

	return nullptr;
}

bool MediaSession::HandleFrame(MediaChannelId channel_id, AVFrame& frame)
{
	if(media_sources_[channel_id]) 
	{
		return media_sources_[channel_id]->HandleFrame(channel_id, frame);
	}
	return false;
}

bool MediaSession::AddClient(SOCKET rtspfd, std::shared_ptr<RtpConnection> rtp_conn)
{
	std::lock_guard<std::recursive_mutex> lock(map_mutex_);

	auto iter = clients_.find (rtspfd);
	if(iter == clients_.end()) {
		std::weak_ptr<RtpConnection> rtp_conn_weak_ptr = rtp_conn;
		clients_.emplace(rtspfd, rtp_conn_weak_ptr);
		for (auto& callback : notify_connected_callbacks_) {
			callback(session_id_, rtp_conn->GetIp(), rtp_conn->GetPort());
		}			
        
		has_new_client_ = true;
		return true;
	}
            
	return false;
}

void MediaSession::RemoveClient(SOCKET rtspfd)
{  
	std::lock_guard<std::recursive_mutex> lock(map_mutex_);

	auto iter = clients_.find(rtspfd);
	if (iter != clients_.end()) {
		auto conn = iter->second.lock();
		if (conn) {
			for (auto& callback : notify_disconnected_callbacks_) {
				callback(session_id_, conn->GetIp(), conn->GetPort());
			}				
		}
		clients_.erase(iter);
	}
}

MediaSessionId MediaSession::GetMediaSessionId() const
{
	return session_id_;
}

uint32_t MediaSession::GetNumClient() const
{
	return (uint32_t)clients_.size();
}

bool MediaSession::IsMulticast() const
{
	return is_multicast_;
}

const std::string& MediaSession::GetMulticastIp() const
{
	return multicast_ip_;
}

uint16_t MediaSession::GetMulticastPort(MediaChannelId channel_id) const
{
	if (channel_id >= MAX_MEDIA_CHANNEL)
	{
		return 0;
	}
	return multicast_port_[channel_id];
}
