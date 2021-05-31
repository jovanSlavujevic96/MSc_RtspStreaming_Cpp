#pragma once

#include <functional>
#include <memory>

#include "Socket.h"

namespace net
{
    
enum EventType
{
	EVENT_NONE   = 0,
	EVENT_IN     = 1,
	EVENT_PRI    = 2,		
	EVENT_OUT    = 4,
	EVENT_ERR    = 8,
	EVENT_HUP    = 16,
	EVENT_RDHUP  = 8192
};

class Channel final
{
public:
	typedef std::function<void()> EventCallback;
    
	Channel() = delete;
	Channel(const Channel&) = delete;
	Channel(SOCKET sockfd);

	~Channel() = default;
    
	void SetReadCallback(const EventCallback& cb);
	void SetWriteCallback(const EventCallback& cb);
	void SetCloseCallback(const EventCallback& cb);
	void SetErrorCallback(const EventCallback& cb);

	SOCKET GetSocket() const;
	int  GetEvents() const;
	void SetEvents(int events);
    
	void EnableReading();
	void EnableWriting();    
	void DisableReading();    
	void DisableWriting();

	bool IsNoneEvent() const;
	bool IsWriting() const; 
	bool IsReading() const;
    
	void HandleEvent(int events);

private:
	EventCallback read_callback_;
	EventCallback write_callback_;
	EventCallback close_callback_;
	EventCallback error_callback_;
    
	SOCKET sockfd_ = 0;
	int events_ = 0;    
};

typedef std::shared_ptr<Channel> ChannelPtr;

}
