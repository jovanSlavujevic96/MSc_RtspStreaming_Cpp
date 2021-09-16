#include "StringPackage.h"

StringPackage::StringPackage(uint16_t max_size) :
	mSmartString{ std::make_unique<char[]>(max_size) },
	mMaxSize{ max_size }
{

}

const char* StringPackage::cData() const
{
	return mSmartString.get();
}

uint16_t StringPackage::getCurrentSize() const
{
	return mCurrentSize;
}

uint16_t StringPackage::getMaxSize() const
{
	return mMaxSize;
}

void StringPackage::setCurrentSize(uint16_t curr_size)
{
	mCurrentSize = curr_size;
}

char* StringPackage::data()
{
	return &mSmartString[0];
}
