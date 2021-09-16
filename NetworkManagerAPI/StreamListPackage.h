#pragma once

#include <vector>
#include <string>

#include "ipackage.h"

class NetworkManager;
class NetworkUser;

#define ARRAY_SIZE 150u

class StreamListPackage : public IPackage
{
public:
    const char* cData() const override;
    uint16_t getCurrentSize() const override;
    uint16_t getMaxSize() const override;

    void appendStream(const char* stream) noexcept(false);

private:
    friend class NetworkManager;
    friend class NetworkUser;
    inline explicit StreamListPackage(uint16_t MaxNumOfStreams) : mStreams{ MaxNumOfStreams } {}

    std::vector<char[ARRAY_SIZE]> mStreams;
    uint16_t currentSize = 0;

    char* data() override;
};