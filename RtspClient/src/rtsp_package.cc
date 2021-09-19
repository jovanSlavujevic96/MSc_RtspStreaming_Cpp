#include "rtsp_package.h"

RtspPackage::RtspPackage(size_t size) :
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

void RtspPackage::clear()
{
	if (mCapacity)
	{
		std::memset(mData.get(), 0, mCapacity);
	}
}

size_t RtspPackage::getCurrentSize() const
{
	return mCurrentSize;
}

size_t RtspPackage::getMaxSize() const
{
	return mCapacity;
}
