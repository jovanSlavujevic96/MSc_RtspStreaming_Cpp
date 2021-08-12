#pragma once

#include <cstdint>
#include <string>

namespace xop
{

class DigestAuthentication final
{
public:
	DigestAuthentication(std::string realm, std::string username, std::string password);
	~DigestAuthentication() = default;

	const std::string& GetRealm() const;
	const std::string& GetUsername() const;
	const std::string& GetPassword() const;

	std::string GetNonce();
	std::string GetResponse(std::string nonce, std::string cmd, std::string url);

private:
	std::string realm_;
	std::string username_;
	std::string password_;

};

}
