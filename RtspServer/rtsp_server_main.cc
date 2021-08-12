#include <cstdint>
#include <string>
#include <iostream>
#include <vector>
#include <thread>
#include <map>

#include "xop/RtspServer.h"
#include "xop/H264Source.h"
#include "xop/rtp.h"

#include "RtspServerUtils/AvH264.h"

void SendFrameThread(xop::RtspServer* rtsp_server, xop::MediaSessionId session_id, xop::MediaChannelId channel_id, int link);

int main()
{
    const char* suffix = "live";
    const std::string ip = "127.0.0.1";
    const uint16_t port = 9090;
    std::string rtsp_url; 

    std::shared_ptr<net::EventLoop>event_loop = std::make_shared<net::EventLoop>();
    std::shared_ptr<xop::RtspServer>server = xop::RtspServer::Create(event_loop.get());

    if (!server->Start("0.0.0.0" /*INADDR_ANY*/, port))
    {
        printf("RTSP Server listen on %hu failed.\n", port);
        return 0;
    }

    xop::MediaSessionId session_id;
    xop::MediaSession* session = xop::MediaSession::CreateNew(suffix);
    std::unique_ptr<std::thread> streamingThread = nullptr;
    rtsp_url = std::string("rtsp://" + ip + ":" + std::to_string(port) + "/" + suffix);

    xop::MediaChannelId channel = xop::channel_0;

    session->AddSource(channel, xop::H264Source::CreateNew(12u));

    if (!session->StartMulticast())
    {
        std::cout << "Start Multicast for session " << session->GetRtspUrlSuffix() << " failed\n";
        return -1;
    }
    std::cout << "session " << session->GetRtspUrlSuffix() << std::endl;
    std::cout << "mcast ip: " << session->GetMulticastIp() << " & port: " << session->GetMulticastPort(channel) << std::endl;

    session->AddNotifyConnectedCallback([](xop::MediaSessionId sessionId, std::string peer_ip, uint16_t peer_port)
        {
            printf("RTSP client connect, ip=%s, port=%hu, session %hu\n", peer_ip.c_str(), peer_port, sessionId);
        });

    session->AddNotifyDisconnectedCallback([](xop::MediaSessionId sessionId, std::string peer_ip, uint16_t peer_port)
        {
            printf("RTSP client disconnect, ip=%s, port=%hu, session %hu\n", peer_ip.c_str(), peer_port, sessionId);
        });

    session_id = server->AddSession(session);
    
    std::cout << "Play URL: " << rtsp_url << std::endl;
    
    streamingThread = std::make_unique<std::thread>( &::SendFrameThread, server.get(), session_id, channel, 0);
    streamingThread->detach();

    while (1)
    {
        net::Timer::Sleep(100);
    }

    return std::getchar();
}

void SendFrameThread(xop::RtspServer* rtsp_server, xop::MediaSessionId session_id, xop::MediaChannelId channel_id, int link)
{
    cv::VideoCapture capture(link);
    if (!capture.isOpened())
    {
        std::cout << "Opening of capture with id = " << link << " has been failed\n.Exit from thread\n";
        return;
    }

    cv::Mat frame;
    AvH264 h264;
    AvH264EncConfig conf;

    conf.bit_rate = 1024;
    conf.width = 640; //(int)capture.get(cv::CAP_PROP_FRAME_WIDTH); //1280 
    conf.height = 480; // (int)capture.get(cv::CAP_PROP_FRAME_HEIGHT); //720
    conf.gop_size = 60;
    conf.max_b_frames = 0;
    conf.frame_rate = 12;

    h264.open(conf);

    std::string windows_name = "frame" + std::to_string(link);

    //const uint32_t BufferSize = 0xFFFF;
    //xop::AVFrame videoFrame(BufferSize);
    xop::AVFrame videoFrame;

    while (1)
    {
        capture >> frame;
        if (frame.data)
        {
            h264.encode(frame, videoFrame);
            videoFrame.timestamp = xop::H264Source::GetTimestamp();
            //memcpy(videoFrame.buffer, pkt->data, videoFrame.size);
            rtsp_server->PushFrame(session_id, channel_id, videoFrame);
        }
        else
        {
            break;
        }

        cv::waitKey(20);
        //cv::imshow(windows_name, frame);
    };
    h264.close();
}
