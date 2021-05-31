#pragma once

#include <memory>

namespace rtsp
{

enum class MediaType
{
	//PCMU = 0,	 
	PCMA = 8,
	H264 = 96,
	AAC  = 37,
	H265 = 265,   
	NONE
};	

enum class FrameType
{
	VIDEO_FRAME_I = 0x01,	  
	VIDEO_FRAME_P = 0x02,
	VIDEO_FRAME_B = 0x03,    
	AUDIO_FRAME   = 0x11,   
};

struct AVFrame
{	
	bool toFree;
	uint8_t* buffer = NULL;
	//std::shared_ptr<uint8_t> buffer = nullptr;
	uint32_t size;
	uint8_t  type;
	uint32_t timestamp;

	AVFrame(uint32_t size) :
		//buffer(new uint8_t[(!size) ? 1 : size], std::default_delete< uint8_t[]>())
		buffer (new uint8_t[(!size) ? 1 : size] )
	{
		this->size = size;
		type = 0;
		timestamp = 0;
		toFree = true;
	}

	AVFrame()
	{
		this->size = 0;
		type = 0;
		timestamp = 0;
		toFree = false;
	}

	~AVFrame()
	{
		if (toFree)
		{
			delete[] buffer;
		}
	}
};

static const int MAX_MEDIA_CHANNEL = 2;

enum MediaChannelId
{
	channel_0,
	channel_1
};

typedef uint32_t MediaSessionId;

}
