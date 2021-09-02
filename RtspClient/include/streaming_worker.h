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
   Q_OBJECT

public:
    StreamingWorker() = default;
    ~StreamingWorker() = default;

	void setRtpClientIp(const char* ip);
	void setRtpClientPort(uint16_t port);
	void setRtpClientCast(bool is_multicast);

	void start() noexcept(false);
	void stop() noexcept(false);

	void initRtpClient() noexcept(false);
	void stopRtpClient();

	bool getRunning() const;
    cv::Size getFrameSize() const;

signals:
    void updateWindow(const cv::Mat& frame);
    void dropError(const char* title, const char* message);
    void dropWarning(const char* title, const char* message);
    void dropInfo(const char* title, const char* message);

private:

	std::string mRtpIp = "";
	uint16_t mRtpPort = 0;
    bool mIsMulticast = true;
	std::unique_ptr<IClient> mRtpClient = nullptr;
	bool mThreadRunning = false;
	std::unique_ptr<AvH264Decoder> mDecoder = nullptr;
	RtpFramePackage mRtpPackage;
	cv::Mat mCvFrame;

protected:
    void run() override final;
};