#pragma once

#include "rtsp_url_info.h"

class RtspParser
{
public:
	constexpr explicit RtspParser() noexcept = default;
	~RtspParser() = default;

	void setRtspUrlInfo(struct RtspUrlInfo* info) noexcept;
	void setStartingMethod() noexcept;
	bool parseRtspUrl(const char* url, struct RtspUrlInfo* rtspUrlInfo) noexcept;
	void formRtspRequest(std::string& request) noexcept(false);
	void parserRtspResponse(const char* response) noexcept(false);
private:
	enum class RtspMethod;
	enum RtspMethod mCurrentRtspMethod = static_cast< RtspMethod>(0);

	struct RtspUrlInfo* mRtspUrlInfo = NULL;
	uint16_t mCSeqCounter = 0;

	void formOptionsRequest(std::string& request);
};
