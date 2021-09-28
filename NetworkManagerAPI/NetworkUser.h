#pragma once

#include <string>
#include <vector>

#include "ctcp_client.h"

#include "StringPackage.h"

class NetworkUser : public CTcpClient
{
public:
	explicit NetworkUser(const char* manager_ip, uint16_t manager_port);
	inline ~NetworkUser() = default;

	void sendMessage(const std::string& msg) noexcept(false);
	void receiveMessage() noexcept(false);

	const std::vector<std::string>& getStreams() const;
	const std::vector<std::string>& getUrls() const;
	bool gotStreams() const;
	void resetStreams();
private:
	StringPackage mRcvPkg;
	std::vector<std::string> mStreams;
	std::vector<std::string> mUrls;
	bool mGotStreams;
};
