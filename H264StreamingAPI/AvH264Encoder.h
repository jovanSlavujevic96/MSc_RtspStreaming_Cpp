#pragma once

#include <cstdint>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavdevice/avdevice.h>
}
#include <opencv2/opencv.hpp>

#include "AvH264EncConfig.h"

class AvH264Encoder
{
public:
    AvH264Encoder();
    ~AvH264Encoder();

    int open(const AvH264EncConfig& h264_config);
    void close();
    AVPacket* encode(const cv::Mat& mat) noexcept;

private:
    AVCodec* cdc_;
    AVCodecContext* cdc_ctx_;
    AVFrame* avf_;
    AVPacket* avp_;
    cv::Size size;
    int32_t frame_size_;
    int32_t pts_;
};
