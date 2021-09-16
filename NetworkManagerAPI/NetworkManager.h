#pragma once

#include <vector>
#include <string>
#include <mutex>

#include "ithread.h"
#include "ctcp_server.h"

#include "StreamListPackage.h"

class NetworkManager : 
	public CTcpServer, 
	public IThread
{
public:
	NetworkManager(uint16_t port, uint16_t num_of_streams) noexcept;
	~NetworkManager();

	void start() noexcept(false);
	void stop() noexcept;

	void appendStream(const std::string& stream);

private:
	void threadEntry() override final;

	StreamListPackage mPackage;
	std::mutex mMutex;
};
