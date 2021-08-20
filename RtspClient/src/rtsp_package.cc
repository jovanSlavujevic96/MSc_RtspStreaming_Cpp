#include "rtsp_package.h"

RtspPackage::RtspPackage(uint16_t package_max_size) :
	mPkgMaxSize{package_max_size},
	mPkgCurrSize{0},
	mData{std::make_unique<char[]>(mPkgMaxSize)}
{

}

const char* RtspPackage::cData() const
{
	return mData.get();
}

uint16_t RtspPackage::getCurrentSize() const
{
	return mPkgCurrSize;
}

uint16_t RtspPackage::getMaxSize() const
{
	return mPkgMaxSize;
}

char* RtspPackage::data()
{
	return mData.get();
}