#include <cstdint>
#include <string>
#include <iostream>
#include <vector>
#include <thread>

#include "xop/RtspServer.h"
#include "xop/H264Source.h"
#include "xop/rtp.h"

#include "H264StreamingAPI/AvH264Encoder.h"
#include "SocketNetworking/socket_net/include/socket_utils.h"

static void SendFrameThread(xop::RtspServer* rtsp_server, xop::MediaSessionId session_id, xop::MediaChannelId channel_id, int link);

int main()
{
    constexpr xop::MediaChannelId channel = xop::channel_0;
    const std::vector<std::string> rtsp_stream_suffix = { "live", "live2" };
    const uint16_t rtsp_port = 9090;
    std::vector<std::string> rtsp_url;
    std::string rtsp_ip;
    try
    {
        rtsp_ip = ::getOwnIpV4Address();
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return -1;
    }

    std::shared_ptr<net::EventLoop>event_loop = std::make_shared<net::EventLoop>();
    std::shared_ptr<xop::RtspServer>server = xop::RtspServer::Create(event_loop.get());
    if (!server->Start("0.0.0.0" /*INADDR_ANY*/, rtsp_port))
    {
        std::cerr << "RTSP Server listen on " << rtsp_port << " failed.\n";
        return -1;
    }

    uint16_t streaming_iterator;
    std::vector<std::thread> streaming_thread;
    std::vector<xop::MediaSessionId> rtsp_session_id;
    std::vector<xop::MediaSession*> rtsp_session;
    for (const std::string& suffix : rtsp_stream_suffix)
    {
        rtsp_session.push_back(xop::MediaSession::CreateNew(suffix));
        rtsp_url.push_back("rtsp://" + rtsp_ip + ":" + std::to_string(rtsp_port) + "/" + suffix);
    }
    streaming_iterator = 0;
    for (xop::MediaSession* session : rtsp_session)
    {
        session->AddSource(channel, xop::H264Source::CreateNew());
        if (!session->StartMulticast())
        {
            std::cerr << "Start Multicast for session " << session->GetRtspUrlSuffix() << " failed\n";
            return -1;
        }
        session->AddNotifyConnectedCallback([&](xop::MediaSessionId sessionId, std::string peer_ip, uint16_t peer_port)
            {
                printf("STREAM=>%s RTSP client connect, ip=%s, port=%hu, session %hu\n", session->GetRtspUrlSuffix().c_str(), peer_ip.c_str(), peer_port, sessionId);
            });
        session->AddNotifyDisconnectedCallback([&](xop::MediaSessionId sessionId, std::string peer_ip, uint16_t peer_port)
            {
                printf("STREAM=>%s RTSP client disconnect, ip=%s, port=%hu, session %hu\n", session->GetRtspUrlSuffix().c_str(), peer_ip.c_str(), peer_port, sessionId);
            });
        rtsp_session_id.push_back(server->AddSession(session));

        std::cout << "session " << session->GetRtspUrlSuffix() << std::endl;
        std::cout << "mcast ip: " << session->GetMulticastIp() << " & port: " << session->GetMulticastPort(channel) << std::endl;
        std::cout << "Play URL: " << rtsp_url[streaming_iterator] << std::endl << std::endl;
        streaming_iterator++;
    }
    streaming_iterator = 0;
    for (xop::MediaSessionId session_id : rtsp_session_id)
    {
        streaming_thread.push_back(std::thread(&::SendFrameThread, server.get(), session_id, channel, streaming_iterator));
        streaming_thread.back().detach();
        streaming_iterator++;
    }
    
    while (1)
    {
        net::Timer::Sleep(100);
    }

    return std::getchar();
}

static void SendFrameThread(xop::RtspServer* rtsp_server, xop::MediaSessionId session_id, xop::MediaChannelId channel_id, int link)
{
    cv::VideoCapture capture(link);
    if (!capture.isOpened())
    {
        std::cout << "Opening of capture with id = " << link << " has been failed\n.Exit from thread\n";
        return;
    }

    cv::Mat frame;
    AvH264Encoder h264;
    AvH264EncConfig conf;
    AVPacket* av_packet;

    conf.bit_rate = 1024;
    conf.width = 640;
    conf.height = 480;
    conf.gop_size = 60;
    conf.max_b_frames = 0;
    conf.frame_rate = 12;

    h264.open(conf);

    xop::AVFrame videoFrame;

    while (1)
    {
        capture >> frame;
        if (frame.data)
        {
            av_packet = h264.encode(frame);
            videoFrame.timestamp = xop::H264Source::GetTimestamp();
            videoFrame.buffer = av_packet->data;
            videoFrame.size = av_packet->size;
            rtsp_server->PushFrame(session_id, channel_id, videoFrame);
        }
        else
        {
            break;
        }
        net::Timer::Sleep(10);
    };
    h264.close();
}
