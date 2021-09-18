#pragma once

#include <memory>

#include "ipackage.h"

class StringPackage : public IPackage
{
public:
	explicit StringPackage(size_t max_size);
	
	const char* cData() const override;
	size_t getCurrentSize() const override;
	size_t getMaxSize() const override;

	void setCurrentSize(size_t curr_size);
private:
	char* data() override;

	size_t mMaxSize;
	size_t mCurrentSize = 0;

	std::unique_ptr<char[]> mSmartString = nullptr;
};
