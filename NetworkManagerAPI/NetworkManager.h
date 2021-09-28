#pragma once

#include <vector>
#include <string>
#include <mutex>

#include "ithread.h"
#include "ctcp_server.h"

struct StreamInfo;

class NetworkManager : 
	public CTcpServer, 
	public IThread
{
public:
	NetworkManager(uint16_t port) noexcept;
	~NetworkManager();

	void start() noexcept(false);
	void stop() noexcept;

	void update(const std::string& stream, const std::string& url, bool is_live, bool is_busy, bool send=true);
	void update(const char* stream, const char* url, bool is_live, bool is_busy, bool send = true);
private:
	void threadEntry() override final;

	std::vector<StreamInfo*> mStreamsInfoList;
	std::string mStreamsInfoMessage;
	std::recursive_mutex mStreamsInfoMutex;
};
