#pragma once

#include <cstdint>

typedef struct AvH264EncConfig_T
{
    int32_t width;
    int32_t height;
    int32_t frame_rate;
    int32_t bit_rate;
    int32_t gop_size;
    int32_t max_b_frames;
} AvH264EncConfig;
