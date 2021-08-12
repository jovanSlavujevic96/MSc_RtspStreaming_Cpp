#pragma once

#include <string>
#include <memory>
#include <cstdint>
#include <functional>
#include <map>

#include "net/Socket.h"

#include "media.h"
#include "rtp.h"

namespace xop
{

class MediaSource
{
public:
	using SendFrameCallback = std::function<bool (MediaChannelId channel_id, const RtpPacket& pkt)>;

	MediaSource() = default;
	virtual ~MediaSource() = default;

	MediaType GetMediaType() const { return media_type_; }

	virtual std::string GetMediaDescription(uint16_t port=0) = 0;
	virtual std::string GetAttribute()  = 0;
	virtual bool HandleFrame(MediaChannelId channelId, AVFrame& frame) = 0;
	
	void SetSendFrameCallback(const SendFrameCallback callback)	{ send_frame_callback_ = callback; }

	uint32_t GetPayloadType() const { return payload_; }
	uint32_t GetClockRate() const { return clock_rate_; }

protected:
	MediaType media_type_ = MediaType::NONE;
	uint32_t  payload_    = 0;
	uint32_t  clock_rate_ = 0;
	SendFrameCallback send_frame_callback_;
};

}
