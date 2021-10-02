#pragma once

#include <string>

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

	int open(const std::string& file) noexcept;
	void close() noexcept;

	AVPacket* read(bool* end_of_stream);
private:
	AVFormatContext* mInputContext;
	AVPacket* mAvPacket;
	int mStreamIndex;
};
