// RTSP Server

#include "rtsp/RtspServer.h"
#include "net/Timer.h"
#include "rtsp/MediaSource/H264Source.h"
#include "rtsp/MediaConverter/H264File.h"
#include <thread>
#include <memory>
#include <iostream>
#include <string>
#include <cstdint>

void SendFrameThread(rtsp::RtspServer* rtsp_server, rtsp::MediaSessionId session_id, H264File* h264_file);

int main()
{
#ifndef VIDEO_FILE_PATH
	std::cout << "Video file path has not defined. Exit app!\n";
	return -1;
#endif

	H264File h264_file;
	if (!h264_file.Open(VIDEO_FILE_PATH)) 
	{
		printf("Open %s failed.\n", VIDEO_FILE_PATH);
		return 0;
	}

	std::string suffix = "live";
	std::string ip = "127.0.0.1";
	uint16_t port = 554;
	std::string rtsp_url = "rtsp://" + ip + ":" + std::to_string(port) + "/" + suffix;

	std::shared_ptr<net::EventLoop> event_loop(new net::EventLoop());
	std::shared_ptr<rtsp::RtspServer> server = rtsp::RtspServer::Create(event_loop.get());

	if (!server->Start("0.0.0.0", port))
	{
		printf("RTSP Server listen on %hu failed.\n", port);
		return 0;
	}

#ifdef AUTH_CONFIG
	server->SetAuthConfig("-_-", "admin", "12345");
#endif

	rtsp::MediaSession* session = rtsp::MediaSession::CreateNew("live");
	//std::cout << "session->StartMulticast :: " << std::boolalpha << session->StartMulticast() << std::endl;

	session->AddSource(rtsp::channel_0, rtsp::H264Source::CreateNew());
	
	session->AddNotifyConnectedCallback([](rtsp::MediaSessionId sessionId, std::string peer_ip, uint16_t peer_port) {
		printf("RTSP client connect, ip=%s, port=%hu \n", peer_ip.c_str(), peer_port);
		});

	session->AddNotifyDisconnectedCallback([](rtsp::MediaSessionId sessionId, std::string peer_ip, uint16_t peer_port) {
		printf("RTSP client disconnect, ip=%s, port=%hu \n", peer_ip.c_str(), peer_port);
		});

	rtsp::MediaSessionId session_id = server->AddSession(session);

	std::thread t1(SendFrameThread, server.get(), session_id, &h264_file);
	t1.detach();

	std::cout << "Play URL: " << rtsp_url << std::endl;

	while (1) 
	{
		net::Timer::Sleep(100);
	}

	return std::getchar();
}

void SendFrameThread(rtsp::RtspServer* rtsp_server, rtsp::MediaSessionId session_id, H264File* h264_file)
{
	const uint32_t buf_size = 2000000;
	bool end_of_frame;
	rtsp::AVFrame videoFrame = { buf_size };
	while (1) 
	{
		end_of_frame = false;
		videoFrame.size = h264_file->ReadFrame((char*)videoFrame.buffer, buf_size, &end_of_frame);
		if (videoFrame.size > 0)
		{
			videoFrame.timestamp = rtsp::H264Source::GetTimestamp();
			rtsp_server->PushFrame(session_id, rtsp::channel_0, videoFrame);
		}
		else 
		{
			break;
		}
		net::Timer::Sleep(40);
	};
}
