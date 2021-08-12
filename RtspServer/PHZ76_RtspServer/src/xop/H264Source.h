#pragma once

#include "MediaSource.h"
#include "rtp.h"

namespace xop
{ 

class H264Source : public MediaSource
{
public:
	static H264Source* CreateNew(uint32_t framerate=25);
	~H264Source() = default;

	void SetFramerate(uint32_t framerate);
	uint32_t GetFramerate() const;

	std::string GetMediaDescription(uint16_t port) override; 
	std::string GetAttribute() override; 

	bool HandleFrame(MediaChannelId channel_id, AVFrame& frame) override;
	static inline constexpr uint32_t GetTimestamp()
	{
		auto time_point = std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::steady_clock::now());
		return (uint32_t)((time_point.time_since_epoch().count() + 500) / 1000 * 90);
	}
	
private:
	H264Source(uint32_t framerate);
	
	RtpPacket rtp_pkt;
	uint32_t framerate_ = 25;
};
	
}
