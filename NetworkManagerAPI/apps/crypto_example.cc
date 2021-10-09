#include <iostream>

#include "NetworkEncryptor.h"
#include "NetworkDecryptor.h"

int main()
{
	NetworkEncryptor enc;
	NetworkDecryptor dec;

	const std::string poruka = "Hello world";
	std::string enc_poruka;
	std::string dec_poruka;

	enc.encryptText(poruka, enc_poruka);
	dec.decryptText(enc_poruka.c_str(), enc_poruka.size(), dec_poruka);

	std::string res = "SUCCESS";
	int ret = 0;
	if (poruka != dec_poruka)
	{
		res = "ERROR";
		ret = -1;
	}
	std::cout << "Result of encryption/decryption: " << res << std::endl;
	return ret;
}
