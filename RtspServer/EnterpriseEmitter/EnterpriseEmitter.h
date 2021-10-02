#pragma once

#include <cstdint>
#include <string>
#include <memory>
#include <vector>

#include <opencv2/opencv.hpp>

#include "xop/RtspServer.h"
#include "xop/MediaSession.h"
#include "OnDemand/OnDemandSession.h"
#include "NetworkManagerAPI/NetworkManager.h"
#include "H264StreamingAPI/AvH264Encoder.h"
#include "H264StreamingAPI/AvH264Writer.h"

class EnterpriseEmitter : public std::enable_shared_from_this<EnterpriseEmitter>
{
public:
	~EnterpriseEmitter() = default;

	static std::shared_ptr<EnterpriseEmitter> createNew(std::shared_ptr<xop::RtspServer> server, std::shared_ptr<NetworkManager> manager, int32_t camera_id,
		std::shared_ptr<AvH264EncConfig> encoder_config, std::string file_path, std::string rtsp_stream_suffix, std::string rtsp_ip, uint16_t port) noexcept(false);

	void start();
	void stop();

private: // methods
	EnterpriseEmitter() = default;

	void liveStreamingLoop();
	void timeStreamElapseLoop();

private:
	int32_t mCameraId = -1;
	cv::VideoCapture mCameraCapture;

	std::shared_ptr<xop::RtspServer> mRtspStreamingServer = nullptr;
	std::shared_ptr<NetworkManager> mNetworkManager = nullptr;

	uint16_t mVideoCounter = 0;
	std::string mCurrentRecordingFile = "";
	std::vector<std::string> mRecordedFiles;

	AvH264Encoder mCameraCaptureEncoder;
	AvH264Writer mCameraCaptureWriter;

	std::shared_ptr<xop::MediaSession> mLiveSession = nullptr;
	xop::MediaSessionId mLiveSessionId = 0;
	std::vector<std::shared_ptr<OnDemandSession>> mOnDemandSessions;

	std::atomic<bool> mThreadRunningFlag;
	std::mutex mMutexRunning;
	std::thread mLiveStreamingThread;
	std::condition_variable mTimeStreamElapseCondition;
	std::thread mTimeStreamElapseThread;
};
