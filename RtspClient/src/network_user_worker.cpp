#include <sstream>

#include "network_user_worker.h"

NetworkUserWorker::NetworkUserWorker() :
	mNetworkPort{ 0 },
	mNetworkIp{""},
	mUser{NULL}
{

}

NetworkUserWorker::~NetworkUserWorker()
{
	if (mUser)
	{
		delete mUser;
	}
}

void NetworkUserWorker::start() noexcept(false)
{
// start thread
	if (QThread::isRunning())
	{
		return;
	}
	QThread::setTerminationEnabled(true);
	QThread::start(Priority::HighestPriority);
}

void NetworkUserWorker::stop(bool drop_info) noexcept
{
// terminate thread running
	if (!QThread::isRunning())
	{
		if (drop_info)
		{
			emit dropInfo("Information", "RTSP Streaming Client is already closed..");
		}
		return;
	}
	QThread::terminate();

// kill Network User
	if (mUser)
	{
		delete mUser;
		mUser = NULL;
	}
}

void NetworkUserWorker::signUp(const std::string& username, const std::string& email, const std::string& password) noexcept(false)
{
    std::stringstream ss;
    ss << "REGISTER_USER\r\n" << "USERNAME=" << username << "\r\n" << "EMAIL=" << email << "\r\n" << "PASSWORD=" << password << "\r\n\r\n";

    *mUser << ss.str();
    mUser->receiveMessage();
}

void NetworkUserWorker::signIn(const std::string& username_email, const std::string& password) noexcept(false)
{
    std::stringstream ss;
    ss << "LOGIN_USER\r\n" << "USERNAME=" << username_email << "\r\n" << "PASSWORD=" << password << "\r\n\r\n";

    *mUser << ss.str();
    mUser->receiveMessage();
}

void NetworkUserWorker::askForList() noexcept(false)
{
    const std::string get_streams_message = "GET_STREAMS\r\n";
    *mUser << get_streams_message;
}

void NetworkUserWorker::run()
{
	while (true)
	{
		try
		{
			mUser->receiveMessage();
		}
		catch (const CSocketException& e)
		{
			emit dropError("Network User failed", e.what());
			break;
		}
		catch (const std::runtime_error& e)
		{
			emit dropError("Bad message", e.what());
			continue;
		}
		if (mUser->gotStreams())
		{
			const std::vector<std::string>& streams = mUser->getStreams();
			const std::vector<std::string>& urls = mUser->getUrls();
			emit dropLists(streams, urls);
			mUser->resetStreams();
		}
	}
}

void NetworkUserWorker::initNetworkUser() noexcept(false)
{
	if (mUser)
	{
		return;
	}
	mUser = new NetworkUser(mNetworkIp.c_str(), mNetworkPort);
	mUser->initClient();
}

void NetworkUserWorker::deinitNetworkUser()
{
    if (!mUser)
    {
        return;
    }
    delete mUser;
    mUser = NULL;
}
