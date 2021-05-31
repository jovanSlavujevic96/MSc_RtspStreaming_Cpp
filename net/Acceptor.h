#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include "Channel.h"
#include "TcpSocket.h"

namespace net
{

typedef std::function<void(SOCKET)> NewConnectionCallback;

class EventLoop;

class Acceptor final
{
public:	
	Acceptor(EventLoop* eventLoop);
	~Acceptor() = default;

	void SetNewConnectionCallback(const NewConnectionCallback& cb);

	int  Listen(std::string ip, uint16_t port);
	void Close();

private:
	void OnAccept();

	EventLoop* event_loop_ = nullptr;
	std::mutex mutex_;
	std::unique_ptr<TcpSocket> tcp_socket_;
	ChannelPtr channel_ptr_;
	NewConnectionCallback new_connection_callback_;
};

}
