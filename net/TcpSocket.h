#pragma once

#include <cstdint>
#include <string>
#include "Socket.h"

namespace net
{
    
class TcpSocket final
{
public:
    TcpSocket(SOCKET sockfd=-1);
    ~TcpSocket() = default;

    SOCKET Create();
    bool   Bind(std::string ip, uint16_t port);
    bool   Listen(int backlog);
    SOCKET Accept();
    bool   Connect(std::string ip, uint16_t port, int timeout = 0);
    void   Close();
    void   ShutdownWrite();
    SOCKET GetSocket() const { return sockfd_; }
    
private:
    SOCKET sockfd_ = -1;
};

}
