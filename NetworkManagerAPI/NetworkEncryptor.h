#pragma once

#include <memory>
#include <string>

class NetworkEncryptor
{
public:
	NetworkEncryptor();
	~NetworkEncryptor();

	void encryptText(const std::string& in, std::string& out);
private:
	class EncryptorImpl;
	std::unique_ptr<EncryptorImpl> mEncryptorPimpl;
};
