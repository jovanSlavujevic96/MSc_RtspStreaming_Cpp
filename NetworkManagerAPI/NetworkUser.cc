#include "NetworkUser.h"

#define MAX_MESSAGE_LEN 1024u

NetworkUser::NetworkUser(const char* manager_ip, uint16_t manager_port, uint16_t max_num_of_streams) :
	CTcpClient{manager_ip, manager_port},
	mRcvPkg{ MAX_MESSAGE_LEN },
	mStreamList{ max_num_of_streams }
{

}

bool NetworkUser::processRequest(const std::string& req, bool expectList) noexcept(false)
{
	*this << req;
	if (expectList)
	{
		uint16_t ret = *this >> &mStreamList;
		ret /= ARRAY_SIZE;
		uint16_t i = 0;
		for (const char* stream : mStreamList.mStreams)
		{
			if (i >= ret)
			{
				break;
			}
			if (!std::strstr(stream, "rtsp://"))
			{
				return false;
			}
			i++;
		}
	}
	else
	{
		*this >> &mRcvPkg;
		if (!std::strstr("OK", mRcvPkg.cData()))
		{
			return false;
		}
	}
	return true;
}

const std::vector<char[ARRAY_SIZE]>& NetworkUser::getListOfStreams() const
{
	return mStreamList.mStreams;
}
