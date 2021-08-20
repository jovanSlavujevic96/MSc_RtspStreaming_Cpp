#include <stdexcept>
#include <sstream>

#include "rtsp_url_info.h"
#include "rtsp_parser.h"

#define RTSP_VERSION "RTSP/1.0"
#define RECURSION_NEWLINE "\r\n"
#define DOUBLE_RECURSION_NEWLINE "\r\n\r\n" 

enum class RtspParser::RtspMethod
{
    OPTIONS = 0, DESCRIBE, SETUP, PLAY, TEARDOWN, GET_PARAMETER,
    RTCP, NONE,
};

static inline constexpr char const* MethodToString[] =
{
    "OPTIONS", "DESCRIBE", "SETUP", "PLAY", "TEARDOWN", "GET_PARAMETER",
    "RTCP", "NONE"
};

void RtspParser::setRtspUrlInfo(struct RtspUrlInfo* info) noexcept
{
    mRtspUrlInfo = info;
}

void RtspParser::setStartingMethod() noexcept
{
    mCSeqCounter = 0;
    mCurrentRtspMethod = RtspMethod::OPTIONS;
}

bool RtspParser::parseRtspUrl(const char* url, struct RtspUrlInfo* rtspUrlInfo) noexcept
{
    if (strncmp(url, "rtsp://", 7) != 0)
    {
        return false;
    }

    // parse url
    constexpr unsigned int cSizeOfString = 64u;
    uint16_t port = 0;
    char ip[64] = { 0 };
    char suffix[64] = { 0 };

    if (sscanf_s(url + 7, "%[^:]:%hu/%s", ip, cSizeOfString, &port, suffix, cSizeOfString) == 3)
    {

    }
    else if (sscanf_s(url + 7, "%[^/]/%s", ip, cSizeOfString, suffix, cSizeOfString) == 2)
    {
        port = 554;
    }
    else
    {
        return false;
    }

    if (rtspUrlInfo)
    {
        rtspUrlInfo->port = port;
        rtspUrlInfo->ip = ip;
        rtspUrlInfo->suffix = suffix;

        if (rtspUrlInfo->url != url)
        {
            rtspUrlInfo->url = url;
        }
    }
    return true;
}

void RtspParser::formRtspRequest(std::string& request) noexcept(false)
{
    if (mRtspUrlInfo)
    {
        throw std::runtime_error("You can't form RTSP Request if mRtspUrlInfo is NULL.\nCall RtspParser::setRtspUrlInfo method before starting to form RTSP requests.");
    }
    switch (mCurrentRtspMethod)
    {
    case RtspMethod::OPTIONS:
        RtspParser::formOptionsRequest(request);
        break;
    case RtspMethod::DESCRIBE:
        break;
    case RtspMethod::SETUP:
        break;
    case RtspMethod::PLAY:
        break;
    case RtspMethod::TEARDOWN:
        break;
    case RtspMethod::GET_PARAMETER:
        break;
    case RtspMethod::RTCP:
        break;
    case RtspMethod::NONE:
        throw std::runtime_error("You can't form RTSP Request in NONE method enum.\nCall RtspParser::setStartingMethod method before starting to form RTSP requests.");
        break;
    }
}

void RtspParser::parserRtspResponse(const char* response) noexcept(false)
{

}

void RtspParser::formOptionsRequest(std::string& request)
{
    std::stringstream ss;
    ss << MethodToString[static_cast<size_t>(RtspMethod::OPTIONS)] << ' ' << mRtspUrlInfo->url << RTSP_VERSION << RECURSION_NEWLINE;
    ss << "CSeq: " << ++mCSeqCounter << RECURSION_NEWLINE;
    ss << "User - Agent: RtspClient / v1.0.0 (by Jovan Slavujevic)" << DOUBLE_RECURSION_NEWLINE;
    request = std::move(ss.str());
}
