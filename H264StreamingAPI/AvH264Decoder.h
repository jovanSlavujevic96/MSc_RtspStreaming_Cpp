#pragma once

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavdevice/avdevice.h>
}

#include <opencv2/opencv.hpp>

class AvH264Decoder
{
private:
    AVCodec* codec = NULL;
    AVCodecContext* context = NULL;
    AVFrame* frame = NULL;
    AVPacket packet;

    int avframe2cvmat(AVFrame* frame, cv::Mat& res);
    AVFrame* cvmat2avframe(const cv::Mat& mat);
public:
    AvH264Decoder() = default;
    ~AvH264Decoder();

    int init();
    int decode(uint8_t* pDataIn, int nInSize, cv::Mat& res);
    void close();
};
