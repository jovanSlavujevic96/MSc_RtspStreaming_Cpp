#pragma once

#include <string>
#include <chrono>

std::time_t getTimestamp();
std::string getLongTimestampStr();
std::string getShortTimestampStr(std::time_t currTimestamp = getTimestamp());
