#pragma once

#include "iworker_socket.h"

class NetworkManager;
class NetworkEncryptor;
class NetworkDecryptor;

class NetworkClientHandler : public IWorkerSocket	
{
public:
	explicit NetworkClientHandler(SOCKET sock_fd, std::unique_ptr<sockaddr_in> sock_addr, std::shared_ptr<NetworkManager> manager);
	~NetworkClientHandler();
	
	void start() noexcept(false);
	void stop() noexcept;

	bool gotAccess() const;
	void sendMessage(const std::string& message) noexcept(false);

private:
	void threadEntry() override;
	void parseMessage(const char* msg) noexcept(false);

	void parseRegisterMessage(const char* end_line) noexcept(false);
	void parseLoginMessage(const char* end_line) noexcept(false);

	enum class eNetworkClientStatus;
	eNetworkClientStatus mClientStatus;
	std::shared_ptr<NetworkManager> mManager;
	std::unique_ptr<NetworkEncryptor> mEncryptor;
	std::unique_ptr<NetworkDecryptor> mDecryptor;
	bool mGotAccess;
};
