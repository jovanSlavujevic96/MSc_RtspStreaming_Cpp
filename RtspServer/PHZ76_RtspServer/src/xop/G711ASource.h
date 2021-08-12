#pragma once

#include "MediaSource.h"
#include "rtp.h"

namespace xop
{

class G711ASource : public MediaSource
{
public:
	static G711ASource* CreateNew();
	~G711ASource() = default;

	uint32_t GetSampleRate() const;
	uint32_t GetChannels() const;

	virtual std::string GetMediaDescription(uint16_t port=0);
	virtual std::string GetAttribute();

	bool HandleFrame(MediaChannelId channel_id, AVFrame& frame);
	static constexpr uint32_t GetTimestamp();

private:
	G711ASource();

	uint32_t samplerate_ = 8000;   
	uint32_t channels_ = 1;       
};

}
