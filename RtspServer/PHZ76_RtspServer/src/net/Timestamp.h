#pragma once

#include <string>
#include <functional>
#include <cstdint>
#include <chrono>
#include <thread>

namespace net
{
    
class Timestamp final
{
public:
    Timestamp();
    ~Timestamp() = default;

    void Reset();
    int64_t Elapsed();

    static std::string Localtime();

private:
    std::chrono::time_point<std::chrono::high_resolution_clock> begin_time_point_;
};

}
