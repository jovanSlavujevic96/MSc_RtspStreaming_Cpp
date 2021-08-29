#pragma once

#include <cstdint>

typedef struct AvH264EncConfig_T
{
    int32_t width = 1280;
    int32_t height = 720;
    int32_t frame_rate = 12;
    int64_t bit_rate = 1024;
    int32_t gop_size = 60;
    int32_t max_b_frames = 0;
} AvH264EncConfig;
