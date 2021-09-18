#pragma once

#include <memory>

#include "ipackage.h"

#define MAX_MESSAGE_LEN 1024u

class RtspPackage : public IPackage
{
public:
	RtspPackage(size_t size = MAX_MESSAGE_LEN);
	~RtspPackage() = default;

	const char* cData() const override;
	char* data() override;

	void setCurrentSize(uint16_t size);

	size_t getCurrentSize() const override;
	size_t getMaxSize() const override;
private:
	size_t mCurrentSize;
	size_t mCapacity;
	std::unique_ptr<char[]> mData;
};
