#pragma once

#include <cstdint>
#include <memory>

#include <opencv2/opencv.hpp>

#include "xop/media.h"

typedef struct AvH264EncConfig_T
{
    int32_t width = 1280;
    int32_t height = 720;
    int32_t frame_rate = 12;
    int64_t bit_rate = 1024;
    int32_t gop_size = 60;
    int32_t max_b_frames = 0;
} AvH264EncConfig;

class AvH264
{
public:
    AvH264();
    ~AvH264();

    int open(const AvH264EncConfig& h264_config);
    void close();
    void encode(cv::Mat& mat, xop::AVFrame& frame) noexcept(false);

private:
    class avh264_impl;
    std::unique_ptr<avh264_impl> avh264_pimpl;
};
