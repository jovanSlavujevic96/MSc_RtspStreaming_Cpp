#pragma once

#include <vector>
#include <string>
#include <mutex>

#include "ithread.h"
#include "ctcp_server.h"

struct OnDemandStreamInfo;
struct LiveStreamInfo;

class NetworkClientHandler;

class NetworkManager : 
	public CTcpServer, 
	public IThread,
	public std::enable_shared_from_this<NetworkManager>
{
public:
	NetworkManager(uint16_t port) noexcept;
	~NetworkManager();

	void start() noexcept(false);
	void stop() noexcept;
	void clearJunk();

	bool connectSql(const std::string& admin_username, const std::string& admin_password);

	bool insertNewUserSql (const std::string& username, const std::string& email, const std::string& password);
	bool checkExistanceSql(const std::string& data, std::string type_of_data) noexcept(false);
	bool checkPasswordSql(const std::string& password, const std::string& data, std::string type_of_data) noexcept(false);

	void sendStreamMessage(NetworkClientHandler* sender) noexcept(false);

	void updateLiveStream(const std::string& stream, const std::string& url);
	void updateOnDemandStream(const std::string& stream, const std::string& url, std::time_t timestamp);
private:
	void threadEntry() override final;

	void updateStreamMessage(bool handle_lock);
	void sendStreamMessage(bool handle_lock);

	std::vector<LiveStreamInfo*> mLiveStreams;
	std::vector<OnDemandStreamInfo*> mOnDemandStreams;
	std::string mStreamsInfoMessage;
	std::recursive_mutex mStreamsInfoMutex;

	class SqlServer;
	std::unique_ptr<SqlServer> mSqlServer;

	class JunkCleaner;
	std::unique_ptr<JunkCleaner> mJunkCleaner;
};
