#include "rtsp_package.h"

RtspPackage::RtspPackage(uint16_t size) :
	mCapacity{size},
	mCurrentSize{0},
	mData{std::make_unique<char[]>(mCapacity)}
{
}

const char* RtspPackage::cData() const
{
	return mData.get();
}

char* RtspPackage::data()
{
	return mData.get();
}

void RtspPackage::setCurrentSize(uint16_t size)
{
	mCurrentSize = size;
}

uint16_t RtspPackage::getCurrentSize() const
{
	return mCurrentSize;
}

uint16_t RtspPackage::getMaxSize() const
{
	return mCapacity;
}
