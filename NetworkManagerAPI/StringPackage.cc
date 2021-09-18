#include "StringPackage.h"

StringPackage::StringPackage(size_t max_size) :
	mSmartString{ std::make_unique<char[]>(max_size) },
	mMaxSize{ max_size }
{

}

const char* StringPackage::cData() const
{
	return mSmartString.get();
}

size_t StringPackage::getCurrentSize() const
{
	return mCurrentSize;
}

size_t StringPackage::getMaxSize() const
{
	return mMaxSize;
}

void StringPackage::setCurrentSize(size_t curr_size)
{
	mCurrentSize = curr_size;
}

char* StringPackage::data()
{
	return &mSmartString[0];
}
