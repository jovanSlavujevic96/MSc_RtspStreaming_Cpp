#include "xop/H264Source.h"
#include "H264StreamingAPI/Timestamp.h"

#include "EnterpriseEmitter.h"

#define MAX_VIDEOS 4u

std::shared_ptr<EnterpriseEmitter> EnterpriseEmitter::createNew(std::shared_ptr<xop::RtspServer> server, std::shared_ptr<NetworkManager> manager, int32_t camera_id,
	std::shared_ptr<AvH264EncConfig> encoder_config, std::string file_path, std::string rtsp_stream_suffix, std::string rtsp_ip, uint16_t port) noexcept(false)
{
	if (!server)
	{
		throw std::runtime_error("EnterpriseEmitter::CreateNew : server is null.");
	}
	else if (!manager)
	{
		throw std::runtime_error("EnterpriseEmitter::CreateNew : manager is null.");
	}
	else if (camera_id < 0)
	{
		throw std::runtime_error("EnterpriseEmitter::CreateNew : bad value for camera_id => " + std::to_string(camera_id) + ".");
	}
	else if (!encoder_config)
	{
		throw std::runtime_error("EnterpriseEmitter::CreateNew : encoder config is null.");
	}
	else if ("" == rtsp_stream_suffix)
	{
		throw std::runtime_error("EnterpriseEmitter::CreateNew : rtsp stream suffix is empty.");
	}

	// alloc EnterpriseEmitter
	std::shared_ptr<EnterpriseEmitter> emitter(new EnterpriseEmitter);
	emitter->mRtspStreamingServer = server;
	emitter->mNetworkManager = manager;
	emitter->mCameraId = camera_id;

	// init recording h264 files
	emitter->mCurrentRecordingFile = "";
	emitter->mVideoCounter = 0;
	if (file_path != "")
	{
		emitter->mCurrentRecordingFile += file_path + "/";
	}
	for (uint16_t i=0; i<MAX_VIDEOS; ++i)
	{
		emitter->mRecordedFiles.push_back(emitter->mCurrentRecordingFile + rtsp_stream_suffix + "_"  + std::to_string(i) + ".h264");
	}
	emitter->mCurrentRecordingFile = emitter->mRecordedFiles[0];

	// init CvVideoCapture
	if (!emitter->mCameraCapture.open(camera_id, cv::CAP_DSHOW))
	{
		throw std::runtime_error("EnterpriseEmitter::CreateNew : Opening of OpenCV video capture(" + std::to_string(camera_id) + ") has been failed.");
	}

	// init AvH264Encoder
	int ret = emitter->mCameraCaptureEncoder.open(encoder_config);
	if (ret != 0)
	{
		throw std::runtime_error("EnterpriseEmitter::CreateNew : Failed to open AvH264Encoder, ret = " + std::to_string(ret) + ".");
	}

	// init AvH264Writer
	emitter->mCameraCaptureWriter.bind(encoder_config);

	// init MediaSession(s)
	const std::string rtsp_url = std::string("rtsp://") + rtsp_ip + ":" + std::to_string(port) + "/";
	emitter->mLiveSession = xop::MediaSession::CreateNewSmart(rtsp_stream_suffix);
	{
		std::string url = rtsp_url + rtsp_stream_suffix;
		emitter->mLiveSession->SetRtspUrl(url);
	}
	emitter->mLiveSessionId = server->AddSession(emitter->mLiveSession);
	if (!emitter->mLiveSessionId)
	{
		throw std::runtime_error("EnterpriseEmitter::CreateNew : Failed to add Live Media Session - " + emitter->mLiveSession->GetRtspUrlSuffix() + " on RTSP Server.");
	}
	emitter->mLiveSession->AddSource(xop::channel_0, xop::H264Source::CreateNew());
	emitter->mLiveSession->StartMulticast();

	for (uint16_t i = 0; i < MAX_VIDEOS; ++i)
	{
		std::string suffix = rtsp_stream_suffix + "_on_demand_" + std::to_string(i);
		std::string url = rtsp_url + suffix;
		std::shared_ptr<OnDemandSession> session = std::make_shared<OnDemandSession>(suffix);
		session->SetRtspUrl(url);
		xop::MediaSessionId id = server->AddSession(session);
		if (!id)
		{
			throw std::runtime_error("EnterpriseEmitter::CreateNew : Failed to add Live Media Session - " + session->GetRtspUrlSuffix() + " on RTSP Server.");
		}
		session->AddSource(xop::channel_0, xop::H264Source::CreateNew());
		emitter->mOnDemandSessions.push_back(session);
	}

	// update network manager
	manager->updateLiveStream(emitter->mLiveSession->GetRtspUrlSuffix(), emitter->mLiveSession->GetRtspUrl());

	return emitter;
}

void EnterpriseEmitter::start()
{
	mMutexRunning.lock();
	mThreadRunningFlag = true;
	mMutexRunning.unlock();

	// start emitter threads
	mLiveStreamingThread = std::thread(&EnterpriseEmitter::liveStreamingLoop, this);
	mTimeStreamElapseThread = std::thread(&EnterpriseEmitter::timeStreamElapseLoop, this);
}

void EnterpriseEmitter::stop()
{
	mMutexRunning.lock();
	mThreadRunningFlag = false;
	mMutexRunning.unlock();
	mTimeStreamElapseCondition.notify_one();

	// wait for emitter threads
	if (mTimeStreamElapseThread.joinable())
	{
		mTimeStreamElapseThread.join();
	}
	if (mLiveStreamingThread.joinable())
	{
		mLiveStreamingThread.join();
	}
}

void EnterpriseEmitter::liveStreamingLoop()
{
	cv::Mat cv_frame;
	AVPacket* av_packet = NULL;
	xop::AVFrame xop_av_frame;
	std::string recording_file = "";
	std::string timestamp;
	int ret;
	while (true)
	{
		{
			std::lock_guard<std::mutex> guard(mMutexRunning);
			if (!mThreadRunningFlag)
			{
				break;
			}
			if (mCurrentRecordingFile != recording_file)
			{
				recording_file = mCurrentRecordingFile;
				mCameraCaptureWriter.close();
				ret = mCameraCaptureWriter.open(recording_file);
				if (ret != 0)
				{
					std::cerr << "EnterpriseEmitter::liveStreamingLoop : failed to open file: "
						<< recording_file << " ret = " << ret << ".\n" << std::flush;
					break;
				}
			}
		}
		mCameraCapture >> cv_frame;
		if (cv_frame.empty())
		{
			break;
		}
		timestamp = getLongTimestampStr();
		cv::putText(cv_frame, timestamp, cv::Point(30, 30), cv::FONT_HERSHEY_COMPLEX_SMALL, 0.8, cv::Scalar(200, 200, 250), 1, cv::LINE_AA);
		av_packet = mCameraCaptureEncoder.encode(cv_frame);
		if (!av_packet)
		{
			continue;
		}
		xop_av_frame.timestamp = xop::H264Source::GetTimestamp();
		xop_av_frame.buffer = av_packet->data;
		xop_av_frame.size = av_packet->size;
		mRtspStreamingServer->PushFrame(mLiveSessionId, xop::channel_0, xop_av_frame);
		mCameraCaptureWriter.write(av_packet);
		net::Timer::Sleep(30);
	}
}

void EnterpriseEmitter::timeStreamElapseLoop()
{
	static constexpr std::chrono::minutes time_to_elapse = std::chrono::minutes{ 15 };
	std::mutex time_elapse_mutex;
	std::unique_lock<std::mutex> time_elapse_lock(time_elapse_mutex);
	bool notified = false;
	while (true)
	{
		{
			std::lock_guard<std::mutex> guard(mMutexRunning);
			if (!mThreadRunningFlag)
			{
				break;
			}
		}
		notified = mTimeStreamElapseCondition.wait_for(time_elapse_lock, time_to_elapse,
			[this]
			{
				return !mThreadRunningFlag;
			});
		if (!notified)
		{
			std::time_t timestamp = getTimestamp();
			const std::string stream_name = mLiveSession->GetRtspUrlSuffix() + "_" + getShortTimestampStr(timestamp);
			mOnDemandSessions[mVideoCounter]->setReadingVideo(mCurrentRecordingFile);
			mNetworkManager->updateOnDemandStream(stream_name, mOnDemandSessions[mVideoCounter]->GetRtspUrl(), timestamp);

			std::lock_guard<std::mutex> guard(mMutexRunning);
			mVideoCounter = (mVideoCounter + 1) % MAX_VIDEOS;
			if (mOnDemandSessions[mVideoCounter]->getClientsReadingVideo(mRecordedFiles[mVideoCounter]))
			{
				mCurrentRecordingFile = mRecordedFiles[mVideoCounter] + ".tmp";
			}
			else
			{
				mCurrentRecordingFile = mRecordedFiles[mVideoCounter];
			}
		}
	}
}
