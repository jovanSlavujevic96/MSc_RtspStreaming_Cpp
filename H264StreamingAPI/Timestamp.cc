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

std::string getShortTimestampStr(std::time_t currTimestamp)
{
    struct tm* timeinfo = NULL;
    struct tm* tmp = NULL;
    char buffer[80];
#if defined(WIN32) || defined(_WIN32)
    timeinfo = (struct tm*)std::malloc(sizeof(*timeinfo));
    if (timeinfo == NULL)
    {
        return "";
    }
    localtime_s(timeinfo, &currTimestamp);
    tmp = timeinfo;
#elif defined(__linux) || defined(__linux__)
    timeinfo = std::localtime(&currTimestamp);
#endif //defined(WIN32) || defined(_WIN32)
    std::strftime(buffer, 80, "%D_%I:%M%p.", timeinfo);
    if (tmp)
    {
        std::free(tmp);
    }
    return buffer;
}
