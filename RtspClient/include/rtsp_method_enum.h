#pragma once

enum class RtspMethod
{
    OPTIONS = 0, DESCRIBE, SETUP, PLAY, TEARDOWN, GET_PARAMETER,
    RTCP, NONE,
};

static inline constexpr char const* MethodToString[] =
{
    "OPTIONS", "DESCRIBE", "SETUP", "PLAY", "TEARDOWN", "GET_PARAMETER",
    "RTCP", "NONE"
};
