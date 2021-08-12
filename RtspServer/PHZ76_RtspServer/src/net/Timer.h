#pragma once

#include <map>
#include <unordered_map>
#include <chrono>
#include <functional>
#include <cstdint>
#include <chrono>
#include <memory>
#include <mutex>
#include <thread>

namespace net
{
    
typedef std::function<bool(void)> TimerEvent;
typedef uint32_t TimerId;

class Timer
{
public:
	Timer(const TimerEvent& event, uint32_t msec);

	static void Sleep(uint32_t msec);
	void SetEventCallback(const TimerEvent& event);
	void Start(int64_t microseconds, bool repeat = false);
	void Stop();

private:
	friend class TimerQueue;

	void SetNextTimeout(int64_t time_point);
	int64_t getNextTimeout() const;

	bool is_repeat_ = false;
	TimerEvent event_callback_ = [] { return false; };
	uint32_t interval_ = 0;
	int64_t  next_timeout_ = 0;
};

class TimerQueue
{
public:
	TimerId AddTimer(const TimerEvent& event, uint32_t msec);
	void RemoveTimer(TimerId timerId);

	int64_t GetTimeRemaining();
	void HandleTimerEvent();

private:
	int64_t GetTimeNow();

	std::mutex mutex_;
	std::unordered_map<TimerId, std::shared_ptr<Timer>> timers_;
	std::map<std::pair<int64_t, TimerId>, std::shared_ptr<Timer>> events_;
	uint32_t last_timer_id_ = 0;
};

}
