#include <QMessageBox>

#include <stdexcept>
#include <sstream>
#include <chrono>
#include <thread>

#include "cudp_client.h"
#include "cudp_mcast_client.h"

#include "rtp_header.h"
#include "mainwindow.h"
#include "streaming_worker.h"

#define MAX_UDP_PAYLOAD_SIZE 0xFFFFu
#define MAX_RETRIES 20u

void StreamingWorker::setRtpClientIp(const char* ip)
{
	mRtpClientIp = ip;
}

void StreamingWorker::setRtpServerIp(const char* ip)
{
	mRtpServerIp = ip;
}

void StreamingWorker::setRtpClientPort(uint16_t port)
{
	mRtpClientPort = port;
}

void StreamingWorker::setRtpServerPort(uint16_t port)
{
	mRtpServerPort = port;
}

void StreamingWorker::setRtpClientCast(bool is_multicast)
{
	mIsMulticast = is_multicast;
}

void StreamingWorker::start() noexcept(false)
{
	if (mThreadRunning)
	{
        emit dropInfo("Information", "RTSP Streaming Client is already running.");
		return;
	}
	QThread::setTerminationEnabled(true);
	QThread::start(Priority::HighestPriority);
	mThreadRunning = true;
}

void StreamingWorker::stop() noexcept(false)
{
	if (!mThreadRunning)
	{
        emit dropInfo("Information", "RTSP Streaming Client is already closed..");
		return;
	}
	mThreadRunning = false;
	QThread::terminate();
}

void StreamingWorker::initRtpClient() noexcept(false)
{
	if (!mDecoder)
	{
		mDecoder = std::make_unique<AvH264Decoder>();
		if (mDecoder->init() != 0)
		{
			throw std::runtime_error("FFMPEG H264 Decoder init failed.");
		}
	}

	if (mRtpServerIp == "" || !mRtpServerPort)
	{
		throw std::runtime_error("Bad RTP Server ip or port.");
	}

	if (mIsMulticast)
	{
		mRtpClient = std::make_unique<CUdpMcastClient>(mRtpServerIp.c_str(), mRtpServerPort);
	}
	else
	{
		if (mRtpClientIp == "" || !mRtpClientPort)
		{
			throw std::runtime_error("Bad RTP Client ip or port.");
		}
		mRtpClient = std::make_unique<CUdpClient>(mRtpClientIp.c_str(), mRtpClientPort, mRtpServerIp.c_str(), mRtpServerPort);
	}

	try
	{
		mRtpClient->initClient();
	}
	catch (...)
	{
		throw;
	}
}

void StreamingWorker::stopRtpClient()
{
	if (mRtpClient)
	{
		IClient* tmp = mRtpClient.release();
		delete tmp;
		mRtpClient = nullptr;
	}
}

bool StreamingWorker::getRunning() const
{
	return mThreadRunning;
}

void StreamingWorker::run()
{
	IClient& rtp_client = *mRtpClient.get();
	int32_t rcv_bytes;
	int decode_ret;
	static uint8_t frame_data[MAX_UDP_PAYLOAD_SIZE];
	const RtpHeader* rtp_header = mRtpPackage.getRtpHeader();
	size_t frame_size = 0;
	bool display_frame = false;
	bool first_frame = false;
    uint8_t rtsp_offset;
    uint8_t retries = MAX_RETRIES;

    while (mThreadRunning)
	{
		try
		{
			rcv_bytes = rtp_client >> &mRtpPackage;
		}
		catch (const CSocketException& e)
        {
            emit dropError("RTSP Client Failed", e.what());
			break;
		}
        if (rcv_bytes <= RTP_HEADER_SIZE)
        {
            goto init;
        }
		if (!rtp_header->marker)
		{
			if (!first_frame)
			{
				first_frame = true;
			}
			rtsp_offset = 2;
		}
		else
		{
			if (!first_frame)
			{
				first_frame = true;
				rtsp_offset = 0;
			}
			display_frame = true;
		}
		std::memcpy(
			frame_data + frame_size,
			mRtpPackage.cData() + RTP_HEADER_SIZE + rtsp_offset,
			(size_t)rcv_bytes - RTP_HEADER_SIZE - rtsp_offset
		);
		frame_size += (size_t)rcv_bytes - RTP_HEADER_SIZE - rtsp_offset;
		if (!display_frame)
		{
			continue;
		}
		mCvFrame = cv::Mat();
		decode_ret = mDecoder->decode(frame_data, (int)frame_size, mCvFrame);
		if (decode_ret < 0)
		{
			std::cout << "Skipped this frame\n";
            retries--;
            if (!retries)
            {
                mDecoder->close();
                mDecoder->init();
                retries = MAX_RETRIES;
            }
			goto init;
		}
		else if (mCvFrame.empty())
		{
            emit dropError("CV Frame Empty", "CV Frame is empty after decoding");
			break;
        }
        emit dropFrame(mCvFrame);
        retries = MAX_RETRIES;
	init:
		frame_size = 0;
		display_frame = false;
		first_frame = false;
    }
    mThreadRunning = false;
}
