#include "H264StreamingAPI/AvH264Reader.h"

#include "RtpConnection.h"
#include "H264Source.h"

#include "OnDemandSession.h"

OnDemandSession::OnDemandSession(const std::string& url) :
	MediaSession(url),
	mRunning{true},
	mReadingVideo("")
{
	MediaSession::is_multicast_ = false;
	MediaSession::AddNotifyConnectedCallback(
		[this](xop::MediaSessionId sessionId, std::string peer_ip, uint16_t peer_port)
		{
			std::map<SOCKET, std::weak_ptr<xop::RtpConnection>>::iterator iter;
			std::lock_guard<std::recursive_mutex> lock(map_mutex_);
			for (iter = clients_.begin(); iter != clients_.end();)
			{
				std::shared_ptr<xop::RtpConnection> conn = iter->second.lock();
				if (conn == nullptr || conn->GetId() < 0)
				{
					clients_.erase(iter++);
					continue;
				}
				if (session_id_ == sessionId && conn->GetIp() == peer_ip && conn->GetPort() == peer_port)
				{
					std::lock_guard<std::mutex> guard(mReadingMutex);
					OnDemandHandler* handler = new OnDemandHandler{ conn, true, std::thread(), mReadingVideo };
					handler->thread = std::thread(&OnDemandSession::onDemandLoop, this, handler);
					mHandler.push_back(handler);
				}
				iter++;
			}
		}
	);

	MediaSession::AddNotifyDisconnectedCallback(
		[this](xop::MediaSessionId sessionId, std::string peer_ip, uint16_t peer_port)
		{
			std::lock_guard<std::mutex> guard(mReadingMutex);
			size_t id = 0;
			OnDemandHandler* handler = NULL;
			for (OnDemandHandler* handler_ : mHandler)
			{
				if (session_id_ == sessionId && handler_->connection->GetIp() == peer_ip && handler_->connection->GetPort() == peer_port)
				{
					break;
				}
				id++;
			}
			if (mHandler.size() == id)
			{
				return;
			}
			handler = mHandler[id];
			mRunningMutex.lock();
			handler->running = false;
			mRunningMutex.unlock();
			handler->thread.join();
			delete handler;
			mHandler.erase(mHandler.begin() + id);
		}
	);
}

OnDemandSession::~OnDemandSession()
{
	mRunningMutex.lock();
	mRunning = false;
	mRunningMutex.unlock();
	for (OnDemandHandler* handler : mHandler)
	{
		handler->thread.join();
		delete handler;
	}
}

void OnDemandSession::setReadingVideo(const std::string& video)
{
	mReadingMutex.lock();
	mReadingVideo = video;
	mReadingMutex.unlock();
}

const std::string& OnDemandSession::getReadingvideo() const
{
	return mReadingVideo;
}

size_t OnDemandSession::getActiveClients()
{
	std::lock_guard<std::mutex> guard(mReadingMutex);
	return mHandler.size();
}

size_t OnDemandSession::getClientsReadingVideo(const std::string& video)
{
	size_t counter = 0;
	std::lock_guard<std::mutex> guard(mReadingMutex);
	for (const OnDemandHandler* handler : mHandler)
	{
		if (handler->reading_video == video)
		{
			counter++;
		}
	}
	return counter;
}

bool OnDemandSession::AddSource(xop::MediaChannelId channel_id, xop::MediaSource* source)
{
	source->SetSendFrameCallback(
		[this](xop::MediaChannelId channel_id, const xop::RtpPacket& pkt, std::shared_ptr<xop::RtpConnection> connection)
		{
			return (connection->SendRtpPacket(channel_id, pkt) > 0);
		}
	);
	media_sources_[channel_id].reset(source);
	return true;
}

bool OnDemandSession::StartMulticast()
{
	return is_multicast_;
}

void OnDemandSession::onDemandLoop(OnDemandHandler* handler)
{
	bool end_of_file = false;
	bool sent;
	AvH264Reader h264_file;
	AVPacket* av_packet = NULL;
	xop::AVFrame video_frame;

	if (h264_file.open(handler->reading_video) != 0)
	{
		std::cerr << "OnDemandSession::onDemandLoop : session: " << session_id_ <<
			" ip: " << handler->connection->GetIp() <<
			" port: " << handler->connection->GetPort() <<
			" failed to open file: " << handler->reading_video << std::endl << std::flush;
		return;
	}

	video_frame.type = 0;
	while (!end_of_file)
	{
		{
			std::lock_guard guard(mRunningMutex);
			if (!mRunning || !handler->running)
			{
				break;
			}
		}
		av_packet = h264_file.read(&end_of_file);
		if (!av_packet)
		{
			continue;
		}
		video_frame.timestamp = xop::H264Source::GetTimestamp();
		video_frame.buffer = av_packet->data;
		video_frame.size = av_packet->size;

		sent = media_sources_[xop::channel_0]->HandleFrame(xop::channel_0, video_frame, handler->connection);
		if (!sent)
		{
			break;
		}
		net::Timer::Sleep(45);
	}
	handler->reading_video = "";
	h264_file.close();
}
