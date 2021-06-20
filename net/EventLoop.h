#pragma once

#include <memory>
#include <atomic>
#include <unordered_map>
#include <functional>
#include <queue>
#include <thread>
#include <mutex>

#include "SelectTaskScheduler.h"
#include "EpollTaskScheduler.h"
#include "Pipe.h"
#include "Timer.h"
#include "RingBuffer.h"

#define TASK_SCHEDULER_PRIORITY_LOW       0
#define TASK_SCHEDULER_PRIORITY_NORMAL    1
#define TASK_SCHEDULER_PRIORITYO_HIGH     2 
#define TASK_SCHEDULER_PRIORITY_HIGHEST   3
#define TASK_SCHEDULER_PRIORITY_REALTIME  4

namespace net
{

class EventLoop final
{
public:
	EventLoop(const EventLoop&) = delete;
	EventLoop &operator = (const EventLoop&) = delete; 
	EventLoop(uint32_t num_threads =1);  //std::thread::hardware_concurrency()
	~EventLoop();

	std::unique_ptr<TaskScheduler>& GetTaskScheduler();

	bool AddTriggerEvent(TriggerEvent callback);
	TimerId AddTimer(TimerEvent timerEvent, uint32_t msec);
	void RemoveTimer(TimerId timerId);	
	void UpdateChannel(ChannelPtr channel);
	void RemoveChannel(ChannelPtr& channel);
	
	void Loop();
	void Quit();

private:
	std::mutex mutex_;
	uint32_t num_threads_ = 1;
	uint32_t index_ = 1;
	std::vector<std::unique_ptr<TaskScheduler>> task_schedulers_;
	std::vector<std::unique_ptr<std::thread>> threads_;
};

}
