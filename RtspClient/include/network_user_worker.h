#pragma once

#include <QThread>

#include "NetworkUser.h"

class NetworkUserWorker : public QThread
{
	Q_OBJECT

private: //fields
    std::string mNetworkIp;
    uint16_t mNetworkPort;
    NetworkUser* mUser;
public:
    explicit NetworkUserWorker();
    ~NetworkUserWorker();

    void initNetworkUser() noexcept(false);
    void deinitNetworkUser();
    void start() noexcept(false);
    void stop(bool drop_info) noexcept;

    void signUp(const std::string& username, const std::string& email, const std::string& password) noexcept(false);
    void signIn(const std::string& username_email, const std::string& password) noexcept(false);
    void askForList() noexcept(false);

//setters
    inline void setNetworkIp(const std::string& ip) { mNetworkIp = ip; }
    inline void setNetworkPort(uint16_t port) { mNetworkPort = port; }

//getters
    constexpr inline const std::string& getNetworkIp() const { return mNetworkIp; }
    constexpr inline uint16_t getNetworkPort() const { return mNetworkPort; }

signals:
    void dropLists(std::vector<std::string> streams, std::vector<std::string> urls);
    void dropError(std::string title, std::string message);
    void dropWarning(std::string title, std::string message);
    void dropInfo(std::string title, std::string message);

private: //methods
    void run() override final;
};
