#pragma once

#include <string>

extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/pixdesc.h>
#include <libavutil/imgutils.h>
}
#include <opencv2/opencv.hpp>

#include "AvH264EncConfig.h"

class AvH264Writer
{
public:
	explicit AvH264Writer(const char* file_name);
	~AvH264Writer();

	constexpr const std::string& getFileName() const;

	void initWriter(const AvH264EncConfig& h264_config) noexcept(false);
	void closeWriter() noexcept(false);
	void writeFrame(AVPacket* packet);
private:
	std::string mFileName;
	AVFormatContext* mOutContext;
	AVStream* mAvStream;
	AVFrame* mAvFrame;
	int64_t mFramePts;
	cv::Mat mCvWritingFrame;
};
