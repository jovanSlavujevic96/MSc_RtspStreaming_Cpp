#pragma once

#include "iworker_socket.h"

class NetworkClientHandler : public IWorkerSocket	
{
public:
	explicit NetworkClientHandler(SOCKET sock_fd, std::unique_ptr<sockaddr_in> sock_addr);
	~NetworkClientHandler();
	
	void start() noexcept(false);
	void stop() noexcept;

private:
	void threadEntry() override;
	void parseMessage(const char* msg);
	void formMessage(std::string& msg);

	enum class eNetworkClientStatus;
	eNetworkClientStatus mClientStatus;
};
