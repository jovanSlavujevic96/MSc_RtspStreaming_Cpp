#pragma once

#include "MediaSource.h"
#include "rtp.h"

namespace rtsp
{

class H265Source : public MediaSource
{
public:
	static H265Source* CreateNew(uint32_t framerate=25);
	~H265Source() = default;

	void Setframerate(uint32_t framerate);
	uint32_t GetFramerate() const;

	virtual std::string GetMediaDescription(uint16_t port=0); 
	virtual std::string GetAttribute(); 

	bool HandleFrame(MediaChannelId channelId, AVFrame& frame);
	static constexpr uint32_t GetTimestamp();

private:
	H265Source(uint32_t framerate);

	uint32_t framerate_ = 25;
};
	
}
