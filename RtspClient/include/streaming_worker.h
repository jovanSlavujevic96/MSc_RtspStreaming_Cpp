#pragma once

#include <string>
#include <memory>

#include <QThread>

#include <opencv2/opencv.hpp>

#include "AvH264Decoder.h"

#include "rtp_frame_package.h"

class IClient;

class MainWindow;

class StreamingWorker : public QThread
{
public:
	~StreamingWorker();

	void setMainWindow(MainWindow* window);
	void setRtpClientIp(const char* ip);
	void setRtpClientPort(uint16_t port);
	void setRtpClientCast(bool is_multicast);

	void start() noexcept(false);
	void stop() noexcept(false);

	void initRtpClient() noexcept(false);
	void stopRtpClient();

	bool getRunning() const;
private:
	friend class MainWindow;
	StreamingWorker() = default;

	void run() override;

	std::string mRtpIp = "";
	uint16_t mRtpPort = 0;
	bool mIsMulticast = true;
	MainWindow* mWindow = NULL;
	std::unique_ptr<IClient> mRtpClient = nullptr;
	bool mThreadRunning = false;
	std::unique_ptr<AvH264Decoder> mDecoder = nullptr;
	RtpFramePackage mRtpPackage;
	cv::Mat mCvFrame;
};
