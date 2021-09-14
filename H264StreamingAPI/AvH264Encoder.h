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
    int allocNewAvFrame();
    void close();
    const AVPacket* encode(const cv::Mat& mat);

private:
    AVCodec* mCdc;
    AVCodecContext* mCdcCtxt;
    AVFrame* mAvFrame;
    AVPacket* mAvPacket;
    cv::Size mCvSize;
    cv::Mat mCvFrameEdited;
    int32_t mFrameSize;
    int32_t mPts;
};
