#include <iostream>
#include <iomanip>  
#include <sstream>

#include "Timestamp.h"

using namespace net;

Timestamp::Timestamp() :
    begin_time_point_(std::chrono::high_resolution_clock::now())
{
}

void Timestamp::Reset()
{
    begin_time_point_ = std::chrono::high_resolution_clock::now();
}

int64_t Timestamp::Elapsed()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - begin_time_point_).count();
}

std::string Timestamp::Localtime()
{
    std::ostringstream stream;
    auto now = std::chrono::system_clock::now();
    time_t tt = std::chrono::system_clock::to_time_t(now);
	
#if defined(WIN32) || defined(_WIN32)
    struct tm tm;
    localtime_s(&tm, &tt);
    stream << std::put_time(&tm, "%F %T");
#elif  defined(__linux) || defined(__linux__) 
    char buffer[200] = {0};
    std::string timeString;
    std::strftime(buffer, 200, "%F %T", std::localtime(&tt));
    stream << buffer;
#endif	
    return stream.str();
}