#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif//_CRT_SECURE_NO_WARNINGS

#include "StreamListPackage.h"

const char* StreamListPackage::cData() const
{
	return (char*)mStreams.data();
}

uint16_t StreamListPackage::getCurrentSize() const
{
	return ARRAY_SIZE * currentSize;
}

uint16_t StreamListPackage::getMaxSize() const
{
	return ARRAY_SIZE * (uint16_t)mStreams.size();
}

char* StreamListPackage::data()
{
	return (char*)mStreams.data();
}

void StreamListPackage::appendStream(const char* stream) noexcept(false)
{
	if (currentSize >= mStreams.size())
	{
		throw std::bad_alloc();
	}
	std::strcpy(mStreams[currentSize], stream);
	currentSize++;
}