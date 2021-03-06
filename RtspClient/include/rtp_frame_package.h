#pragma once

#include <memory>
#include <cstdint>

#include "rtp_header.h"
#include "ipackage.h"

#define MAX_RTP_PAYLOAD_SIZE 1460u
#define RTP_HEADER_SIZE 12u

class RtpFramePackage : public IPackage
{
public:
	RtpFramePackage(size_t size = RTP_HEADER_SIZE + MAX_RTP_PAYLOAD_SIZE);
	~RtpFramePackage() = default;

	const char* cData() const override;
	char* data() override;

	size_t getCurrentSize() const override;
	size_t getMaxSize() const override;

	const RtpHeader* getRtpHeader() const;
private:
	size_t mSize;
	std::unique_ptr<uint8_t[]> mData;
	RtpHeader* mHeaderPtr;
};
