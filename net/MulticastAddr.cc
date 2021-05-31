#include "MulticastAddr.h"

#include <random>

#include "SocketUtil.h"

using namespace net;

MulticastAddr& MulticastAddr::instance()
{
	static MulticastAddr s_multi_addr;
	return s_multi_addr;
}

std::string MulticastAddr::GetAddr()
{
	std::lock_guard<std::mutex> lock(mutex_);
	std::string addr_str;
	struct sockaddr_in addr = { 0 };
	std::random_device rd;

	for (int n = 0; n <= 10; n++) 
	{
		uint32_t range = 0xE8FFFFFF - 0xE8000100;
		addr.sin_addr.s_addr = ::htonl(0xE8000100 + (rd()) % range);
		addr_str = ::inet_ntoa(addr.sin_addr);

		if (addrs_.find(addr_str) != addrs_.end()) 
		{
			addr_str.clear();
		}
		else 
		{
			addrs_.insert(addr_str);
			break;
		}
	}

	return addr_str;
}

void MulticastAddr::Release(const std::string& addr)
{
	std::lock_guard<std::mutex> lock(mutex_);
	addrs_.erase(addr);
}

void MulticastAddr::Release(const char* addr)
{
	std::lock_guard<std::mutex> lock(mutex_);
	addrs_.erase(addr);
}
