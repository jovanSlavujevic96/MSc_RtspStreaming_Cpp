#pragma once

#include <memory>

#include "ipackage.h"

class StringPackage : public IPackage
{
public:
	explicit StringPackage(uint16_t max_size);
	
	const char* cData() const override;
	uint16_t getCurrentSize() const override;
	uint16_t getMaxSize() const override;

	void setCurrentSize(uint16_t curr_size);
private:
	char* data() override;

	uint16_t mMaxSize;
	uint16_t mCurrentSize = 0;

	std::unique_ptr<char[]> mSmartString = nullptr;
};
