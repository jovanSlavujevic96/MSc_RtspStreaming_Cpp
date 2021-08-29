#pragma once

#include <cstdint>

struct RtpHeader
{
    uint8_t csrc : 4;
    uint8_t extension : 1;
    uint8_t padding : 1;
    uint8_t version : 2;
    uint8_t payload : 7;
    uint8_t marker : 1;

    uint16_t seq;
    uint32_t ts;
    uint32_t ssrc;
};
