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
    size_t getCurrentSize() const override;
    size_t getMaxSize() const override;

    const char* getStream(size_t i) const noexcept(false);
    void appendStream(const char* stream) noexcept(false);
private:
    friend class NetworkManager;
    friend class NetworkUser;
    StreamListPackage(size_t max_num_of_streams);
    ~StreamListPackage();

    size_t mCurrentSize = 0;
    size_t mMaxSize = 0;
    char* mStreams = NULL;

    char* data() override;
};
