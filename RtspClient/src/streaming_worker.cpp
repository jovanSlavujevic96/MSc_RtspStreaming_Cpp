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
#define MAX_RETRIES 10u

StreamingWorker::StreamingWorker() :
	mDecoder{std::make_unique<AvH264Decoder>()}
{
	mRtpClientIp = "0.0.0.0"; /*INADDR_ANY*/
	mRtpServerIp = "";
	mRtpClientPort = 0;
	mRtpServerPort = 0;
	mIsMulticast = true;
	mRtpClient = NULL;
}

StreamingWorker::~StreamingWorker()
{
	if (mRtpClient)
	{
		delete mRtpClient;
		mRtpClient = NULL;
	}
}

void StreamingWorker::start() noexcept(false)
{
// init RTP Client
	try
	{
		StreamingWorker::initRtpClient();
	}
	catch (...)
	{
		throw;
	}

// start thread
	if (QThread::isRunning())
	{
        emit dropInfo("Information", "RTSP Streaming Client is already running.");
		return;
	}
	QThread::setTerminationEnabled(true);
	QThread::start(Priority::HighestPriority);
}

void StreamingWorker::stop(bool drop_info) noexcept
{
// terminate thread running
	if (!QThread::isRunning())
	{
		if (drop_info)
		{
			emit dropInfo("Information", "RTSP Streaming Client is already closed..");
		}
		return;
	}
	QThread::terminate();

// kill RTP client
	if (mRtpClient)
	{
		delete mRtpClient;
		mRtpClient = NULL;
	}

// close H264 Decoder
	mDecoder->close();
}

void StreamingWorker::initRtpClient() noexcept(false)
{
	if (mDecoder->init() != 0)
	{
		throw CSocketException(0, "FFMPEG H264 Decoder init failed.");
	}

	if (mRtpServerIp == "" || !mRtpServerPort)
	{
		throw CSocketException(0, "Bad RTP Server ip or port.");
	}

	if (mIsMulticast)
	{
		mRtpClient = new CUdpMcastClient(mRtpServerIp.c_str(), mRtpServerPort);
	}
	else
	{
		if (mRtpClientIp == "" || !mRtpClientPort)
		{
			throw std::runtime_error("Bad RTP Client ip or port.");
		}
		mRtpClient = new CUdpClient(mRtpClientIp.c_str(), mRtpClientPort, mRtpServerIp.c_str(), mRtpServerPort);
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

void StreamingWorker::run()
{
	IClient& rtp_client = *mRtpClient;
	int32_t rcv_bytes;
	int decode_ret;
	static uint8_t frame_data[MAX_UDP_PAYLOAD_SIZE];
	const RtpHeader* rtp_header = mRtpPackage.getRtpHeader();
	size_t frame_size = 0;
	bool display_frame = false;
	bool first_frame = false;
    uint8_t rtsp_offset;
    uint8_t retries = MAX_RETRIES;

	while (true)
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
}
