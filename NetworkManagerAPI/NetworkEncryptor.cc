#ifndef _SILENCE_CXX17_UNCAUGHT_EXCEPTION_DEPRECATION_WARNING
#define _SILENCE_CXX17_UNCAUGHT_EXCEPTION_DEPRECATION_WARNING
#endif//_SILENCE_CXX17_UNCAUGHT_EXCEPTION_DEPRECATION_WARNING

#ifndef CRYPTOPP_DISABLE_UNCAUGHT_EXCEPTION
#define CRYPTOPP_DISABLE_UNCAUGHT_EXCEPTION
#endif//CRYPTOPP_DISABLE_UNCAUGHT_EXCEPTION

/* cryptopp headers */
#include <hex.h>
#include <aes.h>
#include <modes.h>

#include "NetworkEncryptor.h"

/* cryptopp utils constants */
#define CRYPTO_KEY "0123456789abcdef"
#define CRYPTO_IV  "aaaaaaaaaaaaaaaa"

class NetworkEncryptor::EncryptorImpl
{
public:
	EncryptorImpl();

	void encryptText(const std::string& in, std::string& out);
private:
	CryptoPP::AES::Encryption mAesEncryption;
};

NetworkEncryptor::EncryptorImpl::EncryptorImpl() :
	mAesEncryption((byte*)CRYPTO_KEY, CryptoPP::AES::DEFAULT_KEYLENGTH)
{

}

void NetworkEncryptor::EncryptorImpl::encryptText(const std::string& in, std::string& out)
{
	CryptoPP::CBC_Mode_ExternalCipher::Encryption cbcEncryption(mAesEncryption, (byte*)CRYPTO_IV);

	CryptoPP::StreamTransformationFilter stfEncryptor(cbcEncryption, new CryptoPP::StringSink(out));
	stfEncryptor.Put(reinterpret_cast<const unsigned char*>(in.c_str()), in.length() + 1);
	stfEncryptor.MessageEnd();
}

NetworkEncryptor::NetworkEncryptor() :
	mEncryptorPimpl{std::make_unique<EncryptorImpl>()}
{

}

NetworkEncryptor::~NetworkEncryptor() = default;

void NetworkEncryptor::encryptText(const std::string& in, std::string& out)
{
	mEncryptorPimpl->encryptText(in, out);
}
