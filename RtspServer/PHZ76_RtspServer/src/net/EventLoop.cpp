#include "EventLoop.h"

#if defined(WIN32) || defined(_WIN32) 
#include<windows.h>
#endif

#if defined(WIN32) || defined(_WIN32) 
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib,"Iphlpapi.lib")
#endif 

using namespace net;

EventLoop::EventLoop(uint32_t num_threads)
{
	index_ = 1;
	num_threads_ = 1;
	if (num_threads > 0)
	{
		num_threads_ = num_threads;
	}

	this->Loop();
}

EventLoop::~EventLoop()
{
	this->Quit();
}

std::unique_ptr<TaskScheduler>& EventLoop::GetTaskScheduler()
{
	std::lock_guard<std::mutex> locker(mutex_);
	if (task_schedulers_.size() == 1) 
	{
		return task_schedulers_.at(0);
	}
	else 
	{
		std::unique_ptr<TaskScheduler>& task_scheduler = task_schedulers_.at(index_);
		index_++;
		if (index_ >= task_schedulers_.size()) 
		{
			index_ = 1;
		}		
		return task_scheduler;
	}
	static std::unique_ptr<TaskScheduler> null_ = nullptr;
	return null_;
}

void EventLoop::Loop()
{
	std::lock_guard<std::mutex> locker(mutex_);

	if (!task_schedulers_.empty()) {
		return ;
	}

	for (uint32_t n = 0; n < num_threads_; n++) 
	{
#if defined(__linux) || defined(__linux__) 
		task_schedulers_.push_back(std::make_unique<EpollTaskScheduler>(n));
#elif defined(WIN32) || defined(_WIN32) 
		task_schedulers_.push_back(std::make_unique<SelectTaskScheduler>(n));
#endif
		std::unique_ptr<std::thread> thread(new std::thread(&TaskScheduler::Start, task_schedulers_.back().get()));
		if (thread->native_handle())
		{
			threads_.push_back(std::move(thread));
		}
	}

	const int priority = TASK_SCHEDULER_PRIORITY_REALTIME;

	for (std::unique_ptr<std::thread>& iter : threads_) 
	{
#if defined(__linux) || defined(__linux__) 

#elif defined(WIN32) || defined(_WIN32) 
		switch (priority) 
		{
		case TASK_SCHEDULER_PRIORITY_LOW:
			SetThreadPriority(iter->native_handle(), THREAD_PRIORITY_BELOW_NORMAL);
			break;
		case TASK_SCHEDULER_PRIORITY_NORMAL:
			SetThreadPriority(iter->native_handle(), THREAD_PRIORITY_NORMAL);
			break;
		case TASK_SCHEDULER_PRIORITYO_HIGH:
			SetThreadPriority(iter->native_handle(), THREAD_PRIORITY_ABOVE_NORMAL);
			break;
		case TASK_SCHEDULER_PRIORITY_HIGHEST:
			SetThreadPriority(iter->native_handle(), THREAD_PRIORITY_HIGHEST);
			break;
		case TASK_SCHEDULER_PRIORITY_REALTIME:
			SetThreadPriority(iter->native_handle(), THREAD_PRIORITY_TIME_CRITICAL);
			break;
		}
#endif
	}
}

void EventLoop::Quit()
{
	std::lock_guard<std::mutex> locker(mutex_);

	for (std::unique_ptr<TaskScheduler>& iter : task_schedulers_) 
	{
		iter->Stop();
	}
	for (std::unique_ptr<std::thread>& iter : threads_) 
	{
		iter->join();
	}
	task_schedulers_.clear();
	threads_.clear();
}
	
void EventLoop::UpdateChannel(ChannelPtr channel)
{
	std::lock_guard<std::mutex> locker(mutex_);
	if (task_schedulers_.size() > 0) 
	{
		task_schedulers_[0]->UpdateChannel(channel);
	}	
}

void EventLoop::RemoveChannel(ChannelPtr& channel)
{
	std::lock_guard<std::mutex> locker(mutex_);
	if (task_schedulers_.size() > 0) 
	{
		task_schedulers_[0]->RemoveChannel(channel);
	}	
}

TimerId EventLoop::AddTimer(TimerEvent timerEvent, uint32_t msec)
{
	std::lock_guard<std::mutex> locker(mutex_);
	if (task_schedulers_.size() > 0) {
		return task_schedulers_[0]->AddTimer(timerEvent, msec);
	}
	return 0;
}

void EventLoop::RemoveTimer(TimerId timerId)
{
	std::lock_guard<std::mutex> locker(mutex_);
	if (task_schedulers_.size() > 0) 
	{
		task_schedulers_[0]->RemoveTimer(timerId);
	}	
}

bool EventLoop::AddTriggerEvent(TriggerEvent callback)
{   
	std::lock_guard<std::mutex> locker(mutex_);
	if (task_schedulers_.size() > 0) 
	{
		return task_schedulers_[0]->AddTriggerEvent(callback);
	}
	return false;
}
