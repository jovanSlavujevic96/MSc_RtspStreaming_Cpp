#pragma once

#include <memory>

#include "ipackage.h"

#define ONE_KB 1024llu

class RtspPackage : public IPackage
{
public:
    explicit RtspPackage(uint16_t package_max_size=ONE_KB);
    ~RtspPackage() = default;

    const char* cData() const override;
    uint16_t getCurrentSize() const override;
    uint16_t getMaxSize() const override;

private:
    std::unique_ptr<char[]> mData;
    uint16_t mPkgMaxSize;
    uint16_t mPkgCurrSize;
    
    char* data() override;
};
