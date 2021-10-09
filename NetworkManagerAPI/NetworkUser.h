#pragma once

#include <string>
#include <vector>
#include <memory>

#include "ctcp_client.h"

#include "StringPackage.h"

class NetworkEncryptor;
class NetworkDecryptor;

class NetworkUser : public CTcpClient
{
public:
	explicit NetworkUser(const char* manager_ip, uint16_t manager_port);
	inline ~NetworkUser() = default;

	void getStreamsReqest() noexcept(false);
	void signUpRequest(const std::string& username, const std::string& email, const std::string& password) noexcept(false);
	void signInRequest(const std::string& username_email, const std::string& password) noexcept(false);
	void receiveMessage() noexcept(false);

	const std::vector<std::string>& getStreams() const;
	const std::vector<std::string>& getUrls() const;
	bool gotStreams() const;
	void resetStreams();
private:
	void sendRequest(const std::string& message) noexcept(false);

	StringPackage mRcvPkg;
	std::vector<std::string> mStreams;
	std::vector<std::string> mUrls;
	std::unique_ptr<NetworkEncryptor> mEncryptor;
	std::unique_ptr<NetworkDecryptor> mDecryptor;
	bool mGotStreams;
};
