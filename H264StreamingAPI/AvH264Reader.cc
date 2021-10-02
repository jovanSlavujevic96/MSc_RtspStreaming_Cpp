#include "AvH264Reader.h"

AvH264Reader::AvH264Reader()
{
	mInputContext = NULL;
	mAvPacket = NULL;
	mStreamIndex = -1;
}

AvH264Reader::~AvH264Reader()
{
	AvH264Reader::close();
}

int AvH264Reader::open(const std::string& file) noexcept
{
	int ret;

	// open input file context
	ret = avformat_open_input(&mInputContext, file.c_str(), NULL, NULL);
	if (ret < 0)
	{
		return 1;
	}

	// retrive input stream information
	ret = avformat_find_stream_info(mInputContext, NULL);
	if (ret < 0)
	{
		return 2;
	}

	// find primary video stream
	ret = av_find_best_stream(mInputContext, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
	if (ret < 0)
	{
		return 3;
	}
	mStreamIndex = ret;

	mAvPacket = av_packet_alloc();
	if (!mAvPacket)
	{
		return 4;
	}

	return 0;
}

void AvH264Reader::close() noexcept
{
	if (mInputContext)
	{
		avformat_close_input(&mInputContext);
		mInputContext = NULL;
	}

	if (mAvPacket)
	{
		av_packet_free(&mAvPacket);
		mAvPacket = NULL;
	}
}

AVPacket* AvH264Reader::read(bool* end_of_stream)
{
	// read packet from input file
	int ret = av_read_frame(mInputContext, mAvPacket);
	if (ret < 0 && ret != AVERROR_EOF)
	{
		return NULL;
	}
	else if (ret == 0 && mAvPacket->stream_index != mStreamIndex)
	{
		return NULL;
	}
	*end_of_stream = (ret == AVERROR_EOF);
	return mAvPacket;
}
