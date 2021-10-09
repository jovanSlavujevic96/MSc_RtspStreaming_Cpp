#pragma once

#include <string>
#include <memory>

class NetworkDecryptor
{
public:
	NetworkDecryptor();
	~NetworkDecryptor();

	void decryptText(const char* in, size_t in_len, std::string& out);
private:
	class DecryptorImpl;
	std::unique_ptr<DecryptorImpl> mDecryptorPimpl;
};
