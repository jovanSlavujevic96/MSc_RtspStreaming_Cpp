#pragma once

#include <string>
#include <cstdint>

struct RtspUrlInfo
{
// RTSP URL
    std::string url;

// Info parsed from RTSP URL
    std::string ip;     /* @brief RTSP Server IP address */
    uint16_t port;      /* @brief RTSP Server port       */
    std::string suffix; /* @brief RTSP Server suffix     */
};
