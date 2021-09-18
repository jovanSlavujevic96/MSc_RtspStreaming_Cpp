#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif//_CRT_SECURE_NO_WARNINGS

#include "StreamListPackage.h"

#include <cstring>
#include <stdexcept>

StreamListPackage::StreamListPackage(size_t max_num_of_streams)
{
	mMaxSize = max_num_of_streams;
	mCurrentSize = 0;
	mStreams = new char[mMaxSize * ARRAY_SIZE];
}

StreamListPackage::~StreamListPackage()
{
	if (mStreams)
	{
		delete [] mStreams;
	}
}

const char* StreamListPackage::cData() const
{
	return mStreams;
}

size_t StreamListPackage::getCurrentSize() const
{
	return ARRAY_SIZE * mCurrentSize;
}

size_t StreamListPackage::getMaxSize() const
{
	return ARRAY_SIZE * mMaxSize;
}

char* StreamListPackage::data()
{
	return mStreams;
}

const char* StreamListPackage::getStream(size_t i) const noexcept(false)
{
	if (i >= mMaxSize)
	{
		throw std::bad_alloc();
	}
	else if (!std::strstr(&mStreams[i * ARRAY_SIZE], "rtsp://"))
	{
		throw std::runtime_error("Not RTSP stream.");
	}
	return &mStreams[i * ARRAY_SIZE];
}

void StreamListPackage::appendStream(const char* stream) noexcept(false)
{
	if (mCurrentSize >= mMaxSize)
	{
		throw std::bad_alloc();
	}
	std::strcpy(&mStreams[mCurrentSize*ARRAY_SIZE], stream);
	mCurrentSize++;
}
