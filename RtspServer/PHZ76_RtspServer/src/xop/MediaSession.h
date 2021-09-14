#pragma once

#include <memory>
#include <string>
#include <vector>
#include <mutex>
#include <atomic>
#include <cstdint>

#include "net/Socket.h"
#include "net/RingBuffer.h"

#include "media.h"
#include "MediaSource.h"

namespace xop
{

class RtpConnection;

class MediaSession
{
public:
	using Ptr = std::shared_ptr<MediaSession>;
	using NotifyConnectedCallback = std::function<void (MediaSessionId sessionId, std::string peer_ip, uint16_t peer_port)> ;
	using NotifyDisconnectedCallback = std::function<void (MediaSessionId sessionId, std::string peer_ip, uint16_t peer_port)> ;

	static MediaSession* CreateNew(const std::string& url_suffix);
	static MediaSession* CreateNew(const char* url_suffix="live");
	virtual ~MediaSession();

	bool AddSource(MediaChannelId channel_id, MediaSource* source);
	bool RemoveSource(MediaChannelId channel_id);

	bool StartMulticast();

	void AddNotifyConnectedCallback(const NotifyConnectedCallback& callback);
	void AddNotifyDisconnectedCallback(const NotifyDisconnectedCallback& callback);

	const std::string& GetRtspUrlSuffix() const;
	void SetRtspUrlSuffix(const std::string& suffix);

	const std::string& GetRtspUrl() const;
	void SetRtspUrl(const std::string& suffix);
	void SetRtspUrl(const char* suffix);

	std::string GetSdpMessage(std::string ip, std::string session_name ="");
	MediaSource* GetMediaSource(MediaChannelId channel_id);

	bool HandleFrame(MediaChannelId channel_id, AVFrame& frame);

	bool AddClient(SOCKET rtspfd, std::shared_ptr<RtpConnection> rtp_conn);
	void RemoveClient(SOCKET rtspfd);

	MediaSessionId GetMediaSessionId() const;

	uint32_t GetNumClient() const;
	bool IsMulticast() const;
	const std::string& GetMulticastIp() const;
	uint16_t GetMulticastPort(MediaChannelId channel_id) const;

private:
	friend class MediaSource;
	friend class RtspServer;
	MediaSession(const char* url_suffxx);

	MediaSessionId session_id_ = 0;
	std::string url_;
	std::string suffix_;
	std::string sdp_;

	std::vector<std::unique_ptr<MediaSource>> media_sources_;
	std::vector<net::RingBuffer<AVFrame>> buffer_;

	std::vector<NotifyConnectedCallback> notify_connected_callbacks_;
	std::vector<NotifyDisconnectedCallback> notify_disconnected_callbacks_;
	std::mutex mutex_;
	std::mutex map_mutex_;
	std::map<SOCKET, std::weak_ptr<RtpConnection>> clients_;

	bool is_multicast_ = false;
	uint16_t multicast_port_[MAX_MEDIA_CHANNEL];
	std::string multicast_ip_;
	std::atomic_bool has_new_client_;

	static std::atomic_uint last_session_id_;
};

}
