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

#include "NetworkDecryptor.h"

/* cryptopp utils constants */
#define CRYPTO_KEY "0123456789abcdef"
#define CRYPTO_IV  "aaaaaaaaaaaaaaaa"

class NetworkDecryptor::DecryptorImpl
{
public:
	DecryptorImpl();

	void decryptText(const char* in, size_t in_len, std::string& out);
private:
	CryptoPP::AES::Decryption mAesDecryption;

};

NetworkDecryptor::DecryptorImpl::DecryptorImpl() :
	mAesDecryption{ (byte*)CRYPTO_KEY, CryptoPP::AES::DEFAULT_KEYLENGTH }
{

}

void NetworkDecryptor::DecryptorImpl::decryptText(const char* in, size_t in_len, std::string& out)
{
	CryptoPP::CBC_Mode_ExternalCipher::Decryption cbcDecryption(mAesDecryption, (byte*)CRYPTO_IV);
	CryptoPP::StreamTransformationFilter stfDecryptor(cbcDecryption, new CryptoPP::StringSink(out));
	stfDecryptor.Put(reinterpret_cast<const unsigned char*>(in), in_len);
	stfDecryptor.MessageEnd();
}

NetworkDecryptor::NetworkDecryptor() :
	mDecryptorPimpl{std::make_unique<DecryptorImpl>()}
{

}

NetworkDecryptor::~NetworkDecryptor() = default;

void NetworkDecryptor::decryptText(const char* in, size_t in_len, std::string& out)
{
	mDecryptorPimpl->decryptText(in, in_len, out);
	if (out.size() > 1)
	{
		out.resize(out.size() - 1);
	}
}
