#pragma once

#include <memory>

#include "ipackage.h"

#define MAX_MESSAGE_LEN 1024u

class RtspPackage : public IPackage
{
public:
	RtspPackage(uint16_t size = MAX_MESSAGE_LEN);
	~RtspPackage() = default;

	const char* cData() const override;
	char* data() override;

	void setCurrentSize(uint16_t size);

	uint16_t getCurrentSize() const override;
	uint16_t getMaxSize() const override;
private:
	uint16_t mCurrentSize;
	uint16_t mCapacity;
	std::unique_ptr<char[]> mData;
};
