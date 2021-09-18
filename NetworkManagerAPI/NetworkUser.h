#pragma once

#include "ctcp_client.h"

#include "StreamListPackage.h"
#include "StringPackage.h"

class NetworkUser : public CTcpClient
{
public:
	explicit NetworkUser(const char* manager_ip, uint16_t manager_port, uint16_t max_num_of_streams);
	inline ~NetworkUser() = default;

	bool processRequest(const std::string& req, bool expectList = false) noexcept(false);
	const StreamListPackage& getPacakge() const;
private:
	StringPackage mRcvPkg;
	StreamListPackage mStreamList;
};
