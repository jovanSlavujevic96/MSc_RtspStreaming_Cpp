#pragma once

#include <string>
#include <memory>

#include <QThread>

#include <opencv2/opencv.hpp>

#include "AvH264Decoder.h"

#include "rtp_frame_package.h"

class IClient;

class StreamingWorker : public QThread
{
   Q_OBJECT

private: //fields
	std::string mRtpClientIp;
	std::string mRtpServerIp;
	uint16_t mRtpClientPort;
	uint16_t mRtpServerPort;
	bool mIsMulticast;
	IClient* mRtpClient;
	std::unique_ptr<AvH264Decoder> mDecoder;
	RtpFramePackage mRtpPackage;
	cv::Mat mCvFrame;

public:
    explicit StreamingWorker();
    ~StreamingWorker();

// setters
	inline void setRtpClientIp(const char* ip) { mRtpClientIp = ip; }
	inline void setRtpServerIp(const char* ip) { mRtpServerIp = ip; }
	inline void setRtpClientPort(uint16_t port) { mRtpClientPort = port; }
	inline void setRtpServerPort(uint16_t port) { mRtpServerPort = port; }
	inline void IsRtpMulticast(bool is_multicast) { mIsMulticast = is_multicast; }

// getters
	inline constexpr const std::string& getRtpClientIp() const { return mRtpClientIp; }
	inline constexpr const std::string& getRtpServerIp() const { return mRtpServerIp; }
	inline constexpr uint16_t getRtpClientPort() const { return mRtpClientPort; }
	inline constexpr uint16_t getRtpServerPort() const { return mRtpServerPort; }
	inline constexpr bool IsRtpMulticast() const { return mIsMulticast; }

	void start() noexcept(false);
	void stop(bool drop_info = true) noexcept;

signals:
    void dropFrame(cv::Mat frame);
    void dropError(std::string title, std::string message);
    void dropWarning(std::string title, std::string message);
    void dropInfo(std::string title, std::string message);

private: //methods
    void run() override final;
	void initRtpClient() noexcept(false);
};
