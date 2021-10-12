#include <sstream>
#include <stdexcept>
#include <algorithm>

#include "csocket_exception.h"
#include "socket_utils.h"

extern "C"
{
#include <mysql.h>
}

#include "NetworkClientHandler.h"
#include "NetworkManager.h"

struct LiveStreamInfo
{
	std::string stream;
	std::string url;
};

struct OnDemandStreamInfo
{
	std::string stream;
	std::string url;
	std::time_t timestamp;
};

inline constexpr bool sortOnDemandStreamInfo(const OnDemandStreamInfo* left, const OnDemandStreamInfo* right)
{
	return (left->timestamp < right->timestamp);
}

class NetworkManager::SqlServer
{
public:
	SqlServer() = default;
	~SqlServer() = default;

	bool connect(const char* admin_username, const char* admin_password);
	bool insertNewUser(const char* username, const char* email, const char* password);
	bool checkExistance(const char* data, const char* type_of_data) noexcept(false);
	bool checkPassword(const char* password, const char* data, const char* type_of_data) noexcept(false);
private:
	MYSQL* mConnector = NULL;
};

bool NetworkManager::SqlServer::connect(const char* admin_username, const char* admin_password)
{
	mConnector = mysql_init(NULL);
	mConnector = mysql_real_connect(mConnector, "localhost", admin_username, admin_password, "rtsp_network_users", 0, NULL, 0);
	return (mConnector != NULL);
}

bool NetworkManager::SqlServer::insertNewUser(const char* username, const char* email, const char* password)
{
	int qstate;
	std::stringstream sql_format_message;
	std::string sql_message;
	sql_format_message << "INSERT INTO users(username, email, password) VALUES('" << \
		username << "','" << email << "','" << password << "')";
	sql_message = sql_format_message.str();
	qstate = mysql_query(mConnector, sql_message.c_str());
	return (!qstate);
}

bool NetworkManager::SqlServer::checkExistance(const char* data, const char* type_of_data) noexcept(false)
{
	if (std::strcmp(type_of_data, "username") && std::strcmp(type_of_data, "email") && std::strcmp(type_of_data, "password"))
	{
		throw std::runtime_error(std::string("Bad type of data: ") + type_of_data + ".");
	}
	MYSQL_RES* res;
	MYSQL_ROW row;
	std::string message = "SELECT " + std::string(type_of_data) + " FROM users";
	int qstate = mysql_query(mConnector, message.c_str());
	if (qstate)
	{
		throw std::runtime_error("SELECT request has been failed.");
	}
	res = mysql_store_result(mConnector);
	if (NULL == res)
	{
		throw std::runtime_error("STORE result has been failed.");
	}
	row = mysql_fetch_row(res);
	while (row)
	{
		if (!strcmp(row[0], data))
		{
			return true;
		}
		row = mysql_fetch_row(res);
	}
	return false;
}

bool NetworkManager::SqlServer::checkPassword(const char* password, const char* data, const char* type_of_data) noexcept(false)
{
	if (std::strcmp(type_of_data, "username") && std::strcmp(type_of_data, "email"))
	{
		throw std::runtime_error(std::string("Bad type of data: ") + type_of_data + ".");
	}
	MYSQL_RES* res;
	MYSQL_ROW row;
	std::string message = "SELECT password FROM users WHERE " + std::string(type_of_data) + " = '" + std::string(data) + "'";
	int qstate = mysql_query(mConnector, message.c_str());
	if (qstate)
	{
		throw std::runtime_error("SELECT request has been failed.");
	}
	res = mysql_store_result(mConnector);
	if (NULL == res)
	{
		throw std::runtime_error("STORE result has been failed.");
	}
	row = mysql_fetch_row(res);
	if (!strcmp(row[0], password))
	{
		return true;
	}
	return false;
}

class NetworkManager::JunkCleaner : public IThread
{
public:
	JunkCleaner(std::vector<std::unique_ptr<CSocket>>& clients, std::recursive_mutex& clients_mutex);
	~JunkCleaner();

	void stop();
	void clearJunk();
private:
	void threadEntry() override final;

	std::vector<std::unique_ptr<CSocket>>& mNetworkManagerClients;
	std::recursive_mutex& mNetworkManagerClientsMutex;

	std::atomic<bool> mRunning;
	bool mClear;
	std::mutex mRunningMutex;
	std::condition_variable mCondition;
};

NetworkManager::JunkCleaner::JunkCleaner(std::vector<std::unique_ptr<CSocket>>& clients, std::recursive_mutex& clients_mutex) :
	mNetworkManagerClients{ clients },
	mNetworkManagerClientsMutex{ clients_mutex },
	mRunning{ true },
	mClear{ false }
{

}

NetworkManager::JunkCleaner::~JunkCleaner() = default;

void NetworkManager::JunkCleaner::stop()
{
	mRunningMutex.lock();
	mRunning = false;
	mRunningMutex.unlock();
	mCondition.notify_one();
	IThread::join();
}

void NetworkManager::JunkCleaner::clearJunk()
{
	mClear = true;
	mCondition.notify_one();
}

void NetworkManager::JunkCleaner::threadEntry()
{
	std::mutex entry_mutex;
	std::unique_lock<std::mutex> unique_guard(entry_mutex);
	std::vector<size_t> indexes;
	constexpr std::chrono::duration duration = std::chrono::seconds(1);
	while (true)
	{
		{
			std::lock_guard<std::mutex> guard(mRunningMutex);
			if (!mRunning)
			{
				break;
			}
		}
		indexes.clear();
		for (size_t i = 0; i < mNetworkManagerClients.size(); i++)
		{
			NetworkClientHandler* network_client = dynamic_cast<NetworkClientHandler*>(mNetworkManagerClients[i].get());
			if (network_client && !network_client->isAlive())
			{
				indexes.push_back(i);
			}
		}
		mNetworkManagerClientsMutex.lock();
		for (size_t index : indexes)
		{
			mNetworkManagerClients.erase(mNetworkManagerClients.begin() + index);
		}
		mNetworkManagerClientsMutex.unlock();
		mCondition.wait_for(unique_guard, duration, [this]() { return (!mRunning || mClear); });
		mClear = false;
	}
}

NetworkManager::NetworkManager(uint16_t port) noexcept :
	CTcpServer{port},
	mSqlServer{new SqlServer},
	mJunkCleaner{new JunkCleaner(CTcpServer::mClientSockets, CTcpServer::mClientSocketsMutex)}
{
	CTcpServer::mAllocSocketFunction = [this](SOCKET fd, std::unique_ptr<sockaddr_in> addr) -> std::unique_ptr<CSocket>
		{ 
			return std::make_unique<NetworkClientHandler>(fd, std::move(addr), shared_from_this());
		};
}

NetworkManager::~NetworkManager()
{
	NetworkManager::stop();

	for (OnDemandStreamInfo* stream_info : mOnDemandStreams)
	{
		delete stream_info;
	}

	for (LiveStreamInfo* stream_info : mLiveStreams)
	{
		delete stream_info;
	}
}

void NetworkManager::start() noexcept(false)
{
	if (!CSocket::mRunningSockAddr)
	{
		throw std::runtime_error("NetworkManager::start : Cannot start thread before initServer call.");
	}
	IThread::start();
}

void NetworkManager::stop() noexcept
{
	NetworkClientHandler* network_client = NULL;
	for (std::unique_ptr<CSocket>& client : CTcpServer::mClientSockets)
	{
		network_client = dynamic_cast<NetworkClientHandler*>(client.get());
		if (network_client)
		{
			network_client->stop();
		}
	}
	mJunkCleaner->stop();

	CSocket::closeSocket();
	IThread::join();
}

void NetworkManager::clearJunk()
{
	mJunkCleaner->clearJunk();
}

bool NetworkManager::connectSql(const std::string& admin_username, const std::string& admin_password)
{
	return mSqlServer->connect(admin_username.c_str(), admin_password.c_str());
}

bool NetworkManager::insertNewUserSql(const std::string& username, const std::string& email, const std::string& password)
{
	return mSqlServer->insertNewUser(username.c_str(), email.c_str(), password.c_str());
}

bool NetworkManager::checkExistanceSql(const std::string& data, std::string type_of_data) noexcept(false)
{
	return mSqlServer->checkExistance(data.c_str(), type_of_data.c_str());
}

bool NetworkManager::checkPasswordSql(const std::string& password, const std::string& data, std::string type_of_data) noexcept(false)
{
	return mSqlServer->checkPassword(password.c_str(), data.c_str(), type_of_data.c_str());
}

void NetworkManager::sendStreamMessage(NetworkClientHandler* sender) noexcept(false)
{
	mStreamsInfoMutex.lock();
	sender->sendMessage(mStreamsInfoMessage);
	mStreamsInfoMutex.unlock();
}

void NetworkManager::updateLiveStream(const std::string& stream, const std::string& url)
{
	LiveStreamInfo* tmp = NULL;
	mStreamsInfoMutex.lock();
	for (LiveStreamInfo* stream_info : mLiveStreams)
	{
		if (stream_info->stream == stream)
		{
			tmp = stream_info;
			break;
		}
	}
	if (tmp)
	{
		goto unlock;
	}
	mLiveStreams.push_back(new LiveStreamInfo{ stream, url });

	NetworkManager::updateStreamMessage(false /*handle_lock*/);
	NetworkManager::sendStreamMessage(false /*handle_lock*/);
unlock:
	mStreamsInfoMutex.unlock();
}

void NetworkManager::updateOnDemandStream(const std::string& stream, const std::string& url, std::time_t timestamp)
{
	OnDemandStreamInfo* tmp = NULL;
	mStreamsInfoMutex.lock();
	for (OnDemandStreamInfo* stream_info : mOnDemandStreams)
	{
		if (stream_info->url == url)
		{
			tmp = stream_info;
			break;
		}
	}
	if (!tmp)
	{
		tmp = new OnDemandStreamInfo;
		tmp->url = url;
		mOnDemandStreams.push_back(tmp);
	}
	tmp->stream = stream;
	tmp->timestamp = timestamp;

	NetworkManager::updateStreamMessage(false /*handle_lock*/);
	NetworkManager::sendStreamMessage(false /*handle_lock*/);
	mStreamsInfoMutex.unlock();
}

void NetworkManager::threadEntry()
{
	NetworkClientHandler* network_client = NULL;
	size_t id;
	while (true)
	{
		try
		{
			id = CTcpServer::acceptClient();
			network_client = dynamic_cast<NetworkClientHandler*>(mClientSockets.at(id).get());
			if (network_client)
			{
				network_client->start();
			}
		}
		catch (const CSocketException&)
		{
			break;
		}
	}
}

void NetworkManager::updateStreamMessage(bool handle_lock)
{
	if (handle_lock)
	{
		mStreamsInfoMutex.lock();
	}
	mStreamsInfoMessage = "CMD:STREAM_LIST\r\n";

	for (LiveStreamInfo* stream_info : mLiveStreams)
	{
		mStreamsInfoMessage += "LIVE:" + stream_info->stream + "\r\n";
		mStreamsInfoMessage += stream_info->url + "\r\n";
	}

	std::sort(mOnDemandStreams.begin(), mOnDemandStreams.end(), ::sortOnDemandStreamInfo);

	for (OnDemandStreamInfo* stream_info : mOnDemandStreams)
	{
		mStreamsInfoMessage += stream_info->stream + "\r\n";
		mStreamsInfoMessage += stream_info->url + "\r\n";
	}
	mStreamsInfoMessage += "\r\n";
	if (handle_lock)
	{
		mStreamsInfoMutex.unlock();
	}
}

void NetworkManager::sendStreamMessage(bool handle_lock)
{
	if (handle_lock)
	{
		mStreamsInfoMutex.lock();
	}
	NetworkClientHandler* handler;
	for (const std::unique_ptr<CSocket>& sender : CTcpServer::mClientSockets)
	{
		handler = (NetworkClientHandler*)sender.get();
		if (!handler || !handler->gotAccess())
		{
			continue;
		}
		try
		{
			handler->sendMessage(mStreamsInfoMessage);
		}
		catch (...)
		{

		}
	}
	if (handle_lock)
	{
		mStreamsInfoMutex.unlock();
	}
}
