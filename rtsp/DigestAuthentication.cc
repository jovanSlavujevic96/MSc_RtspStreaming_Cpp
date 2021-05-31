/*3rd party*/
#include "md5/md5.hpp" 

#include "DigestAuthentication.h"

using namespace rtsp;

DigestAuthentication::DigestAuthentication(std::string realm, std::string username, std::string password) :
	realm_(realm),
	username_(username),
	password_(password)
{

}

const std::string& DigestAuthentication::GetRealm() const
{
	return realm_;
}

const std::string& DigestAuthentication::GetUsername() const
{
	return username_;
}

const std::string& DigestAuthentication::GetPassword() const
{
	return password_;
}

std::string DigestAuthentication::GetNonce()
{
	return md5::generate_nonce();
}

std::string DigestAuthentication::GetResponse(std::string nonce, std::string cmd, std::string url)
{
	//md5(md5(<username>:<realm> : <password>) :<nonce> : md5(<cmd>:<url>))

	auto hex1 = md5::md5_hash_hex(username_ + ":" + realm_ + ":" + password_);
	auto hex2 = md5::md5_hash_hex(cmd + ":" + url);
	auto response = md5::md5_hash_hex(hex1 + ":" + nonce + ":" + hex2);
	return response;
}
