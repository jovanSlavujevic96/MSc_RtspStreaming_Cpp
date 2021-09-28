#pragma once

extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/pixdesc.h>
}

class AvH264Reader
{
public:
	AvH264Reader();
	~AvH264Reader();

	int initReader(const char* fileName);
	void closeReader();

	AVPacket* readPacket(bool* end_of_stream);
private:
	AVFormatContext* mInputContext;
	AVPacket* mAvPacket;
	int mStreamIndex;
};
