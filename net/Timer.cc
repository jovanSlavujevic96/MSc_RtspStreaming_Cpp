#include <iostream>

#include "Timer.h"

using namespace net;

//Timer

Timer::Timer(const TimerEvent& event, uint32_t msec) :
	event_callback_(event),
	interval_(msec)
{
	if (interval_ == 0)
	{
		interval_ = 1;
	}
}

void Timer::Sleep(uint32_t msec)
{
	std::this_thread::sleep_for(std::chrono::milliseconds(msec));
}

void Timer::SetEventCallback(const TimerEvent& event)
{
	event_callback_ = event;
}

void Timer::Start(int64_t microseconds, bool repeat)
{
	is_repeat_ = repeat;
	auto time_begin = std::chrono::high_resolution_clock::now();
	int64_t elapsed = 0;

	do
	{
		std::this_thread::sleep_for(std::chrono::microseconds(microseconds - elapsed));
		time_begin = std::chrono::high_resolution_clock::now();
		if (event_callback_)
		{
			event_callback_();
		}
		elapsed = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - time_begin).count();
		if (elapsed < 0)
		{
			elapsed = 0;
		}
	} while (is_repeat_);
}

void Timer::Stop()
{
	is_repeat_ = false;
}

void Timer::SetNextTimeout(int64_t time_point)
{
	next_timeout_ = time_point + interval_;
}

int64_t Timer::getNextTimeout() const
{
	return next_timeout_;
}

//TimerQueue

TimerId TimerQueue::AddTimer(const TimerEvent& event, uint32_t ms)
{    
	std::lock_guard<std::mutex> locker(mutex_);

	int64_t timeout = GetTimeNow();
	TimerId timer_id = ++last_timer_id_;

	auto timer = std::make_shared<Timer>(event, ms);	
	timer->SetNextTimeout(timeout);
	timers_.emplace(timer_id, timer);
	events_.emplace(std::pair<int64_t, TimerId>(timeout + ms, timer_id), std::move(timer));
	return timer_id;
}

void TimerQueue::RemoveTimer(TimerId timerId)
{
	std::lock_guard<std::mutex> locker(mutex_);

	auto iter = timers_.find(timerId);
	if (iter != timers_.end())
	{
		int64_t timeout = iter->second->getNextTimeout();
		events_.erase(std::pair<int64_t, TimerId>(timeout, timerId));
		timers_.erase(timerId);
	}
}

int64_t TimerQueue::GetTimeNow()
{
	auto time_point = std::chrono::steady_clock::now();	
	return std::chrono::duration_cast<std::chrono::milliseconds>(time_point.time_since_epoch()).count();
}

int64_t TimerQueue::GetTimeRemaining()
{
	std::lock_guard<std::mutex> locker(mutex_);

	if (timers_.empty())
	{
		return -1;
	}

	int64_t msec = events_.begin()->first.first - GetTimeNow();
	if (msec < 0)
	{
		msec = 0;
	}

	return msec;
}

void TimerQueue::HandleTimerEvent()
{
	if(!timers_.empty())
	{
		std::lock_guard<std::mutex> locker(mutex_);
		int64_t timePoint = GetTimeNow();
		while(!timers_.empty() && events_.begin()->first.first<=timePoint)
		{	
			TimerId timerId = events_.begin()->first.second;
			bool flag = events_.begin()->second->event_callback_();
			if(flag == true)
			{
				events_.begin()->second->SetNextTimeout(timePoint);
				auto timerPtr = std::move(events_.begin()->second);
				events_.erase(events_.begin());
				events_.emplace(std::pair<int64_t, TimerId>(timerPtr->getNextTimeout(), timerId), timerPtr);
			}
			else
			{		
				events_.erase(events_.begin());
				timers_.erase(timerId);				
			}
		}	
	}
}
