#include "NetworkUser.h"

#define MAX_MESSAGE_LEN 2048u //2kb

#define CMD_PREFFIX_WORD "CMD:"
#define CMD_STREAM_LIST "CMD:STREAM_LIST\r\n"

#define RESP_PREFFIX_WORD "RESP:"
#define RESP_OK "RESP:OK\r\n"
#define RESP_ERR "RESP:ERR\r\n"

static const uint16_t cCmdStreamListLen = std::strlen(CMD_STREAM_LIST);

static inline void SetStreamsAndUrls(const char* str, std::vector<std::string>& streams, std::vector<std::string>& urls) noexcept(false)
{
	const char* iterator = str;
	const char* end = std::strstr(str, "\r\n\r\n");
	if (!end)
	{
		throw std::runtime_error("Bad message format. Missing \"\\r\\n\\r\\n\".");
	}
	streams.clear();
	urls.clear();
	while (iterator != end)
	{
		if (iterator != str)
		{
			iterator += 2;
		}
		const char* tmp = std::strstr(iterator, "\r\n");
		if (!tmp)
		{
			throw std::runtime_error("Impossible use-case.");
		}
		if (streams.size() == urls.size())
		{
			streams.push_back(std::string(iterator, tmp));
		}
		else
		{
			urls.push_back(std::string(iterator, tmp));
		}
		iterator = tmp;
	}
}

NetworkUser::NetworkUser(const char* manager_ip, uint16_t manager_port) :
	CTcpClient{manager_ip, manager_port},
	mRcvPkg{ MAX_MESSAGE_LEN },
	mGotStreams{false}
{

}

void NetworkUser::sendMessage(const std::string& msg) noexcept(false)
{
	*this << msg;
}

void NetworkUser::receiveMessage() noexcept(false)
{
	*this >> &mRcvPkg;

	if (std::strstr(mRcvPkg.cData(), CMD_PREFFIX_WORD))
	{
		if (std::strstr(mRcvPkg.cData(), CMD_STREAM_LIST))
		{
			const char*  iterator = mRcvPkg.cData() + cCmdStreamListLen;
			::SetStreamsAndUrls(iterator, mStreams, mUrls);
			mGotStreams = true;
		}
	}
	else if (std::strstr(mRcvPkg.cData(), RESP_PREFFIX_WORD))
	{
		if (!std::strstr(mRcvPkg.cData(), RESP_OK))
		{
			throw std::runtime_error("Bad message format. RESP message is not OK.");
		}
	}
	else
	{
		throw std::runtime_error("Bad message format. Message is nor CMD nor RESP.");
	}
}

const std::vector<std::string>& NetworkUser::getStreams() const
{
	return mStreams;
}

const std::vector<std::string>& NetworkUser::getUrls() const
{
	return mUrls;
}

bool NetworkUser::gotStreams() const
{
	return mGotStreams;
}

void NetworkUser::resetStreams()
{
	mGotStreams = false;
}
