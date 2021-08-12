#include "Channel.h"

using namespace net;

Channel::Channel(SOCKET sockfd) : 
	sockfd_(sockfd)
{
}

void Channel::SetReadCallback(const EventCallback& cb)
{
	read_callback_ = cb;
}

void Channel::SetWriteCallback(const EventCallback& cb)
{
	write_callback_ = cb;
}

void Channel::SetCloseCallback(const EventCallback& cb)
{
	close_callback_ = cb;
}

void Channel::SetErrorCallback(const EventCallback& cb)
{
	error_callback_ = cb;
}

SOCKET Channel::GetSocket() const 
{ 
	return sockfd_; 
}

int Channel::GetEvents() const 
{ 
	return events_; 
}

void Channel::SetEvents(int events) 
{ 
	events_ = events; 
}

void Channel::EnableReading()
{
	events_ |= EVENT_IN;
}

void Channel::EnableWriting()
{
	events_ |= EVENT_OUT;
}

void Channel::DisableReading()
{
	events_ &= ~EVENT_IN;
}

void Channel::DisableWriting()
{
	events_ &= ~EVENT_OUT;
}

bool Channel::IsNoneEvent() const 
{ 
	return events_ == EVENT_NONE; 
}

bool Channel::IsWriting() const 
{ 
	return (events_ & EVENT_OUT) != 0; 
}

bool Channel::IsReading() const
{ 
	return (events_ & EVENT_IN) != 0; 
}

void Channel::HandleEvent(int events)
{
	if (events & (EVENT_PRI | EVENT_IN)) 
	{
		read_callback_();
	}

	if (events & EVENT_OUT) 
	{
		write_callback_();
	}

	if (events & EVENT_HUP) 
	{
		close_callback_();
		return;
	}

	if (events & (EVENT_ERR)) 
	{
		error_callback_();
	}
}
