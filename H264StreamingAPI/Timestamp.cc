#include "Timestamp.h"

std::time_t getTimestamp()
{
    return std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
}

std::string getLongTimestampStr()
{
    std::time_t currTimestamp = getTimestamp();
#if defined(WIN32) || defined(_WIN32)
    char timestamp[1024u];
    ctime_s(timestamp, 1024u, &currTimestamp);
    timestamp[std::strlen(timestamp) - 1] = '\0';
    return timestamp;
#elif defined(__linux) || defined(__linux__)
    return std::ctime(&currTimestamp);
#endif //defined(WIN32) || defined(_WIN32)
}

std::string getShortTimeStampStr()
{
    std::time_t currTimestamp = getTimestamp();
    struct tm* timeinfo;
    char buffer[80];

    timeinfo = localtime(&currTimestamp);

    strftime(buffer, 80, "%D_%I:%M%p.", timeinfo);
    return buffer;
}
