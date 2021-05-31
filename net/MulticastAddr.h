#pragma once

#include <string>
#include <mutex>
#include <unordered_set>

namespace net
{

class MulticastAddr
{
public:
	static MulticastAddr& instance();

	std::string GetAddr();

	void Release(const std::string& addr);
	void Release(const char* addr);

private:
	std::mutex mutex_;
	std::unordered_set<std::string> addrs_;
};

}
