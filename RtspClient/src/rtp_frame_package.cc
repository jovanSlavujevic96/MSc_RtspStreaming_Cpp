#include "rtp_frame_package.h"

RtpFramePackage::RtpFramePackage(size_t size) :
	mSize{size}
{
	mData = std::make_unique<uint8_t[]>(mSize);
	mHeaderPtr = (RtpHeader*)mData.get();
}

const char* RtpFramePackage::cData() const
{
	return (char*)mData.get();
}

char* RtpFramePackage::data()
{
	return (char*)mData.get();
}

size_t RtpFramePackage::getCurrentSize() const
{
	return mSize;
}

size_t RtpFramePackage::getMaxSize() const
{
	return mSize;
}

const RtpHeader* RtpFramePackage::getRtpHeader() const
{
	return mHeaderPtr;
}
