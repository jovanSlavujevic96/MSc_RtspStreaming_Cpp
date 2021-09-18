#include <cstdint>
#include <string>
#include <iostream>
#include <vector>
#include <thread>
#include <map>
#include <mutex>

#include "xop/RtspServer.h"
#include "xop/H264Source.h"
#include "xop/rtp.h"

#include "H264StreamingAPI/AvH264Encoder.h"
#include "H264StreamingAPI/AvH264Writer.h"
#include "NetworkManagerAPI/NetworkManager.h"
#include "SocketNetworking/socket_net/include/socket_utils.h"

#define DEFAULT_WIDTH 640u
#define DEFAULT_HEIGHT 480u

#define DEFAULT_NETWORK_MANAGER_PORT 9089u
#define DEFAULT_RTSP_SERVER_PORT 9090u

static std::atomic<bool> bExitThreads = false;
static std::mutex mutex;

static void LiveStreamThread(cv::VideoCapture* video_capture, AvH264Encoder* h264_encoder, AvH264Writer* h264_writer, xop::RtspServer* rtsp_server, 
    xop::MediaSessionId session_id, xop::MediaChannelId channel_id, int link);

int __cdecl main()
{
    // RTSP info variables
    constexpr uint16_t rtsp_port = DEFAULT_RTSP_SERVER_PORT;
    std::string rtsp_ip;
    std::map<const char*, uint16_t> rtsp_stream_info_map;

    // Video processing variables
    AvH264EncConfig conf;
    std::vector<std::unique_ptr<AvH264Encoder>> h264_encoder;
    std::vector<cv::VideoCapture> webcam_capture;
    std::vector<std::unique_ptr<AvH264Writer>> h264_writer;
    std::vector<uint16_t> video_counter;

    // RTSP/RTP handler variables
    net::EventLoop event_loop;
    std::shared_ptr<xop::RtspServer>server = xop::RtspServer::Create(&event_loop);
    std::vector<xop::MediaSessionId> rtsp_session_id;
    std::vector<xop::MediaSession*> rtsp_session;
    std::vector<std::thread> streaming_thread;
    uint16_t streaming_iterator;

    // Network Manager variables/constants
    constexpr uint16_t manager_port = DEFAULT_NETWORK_MANAGER_PORT;
    std::unique_ptr<NetworkManager> manager = nullptr;

    // init list of streams
    rtsp_stream_info_map.emplace("camera_0", 0);
    rtsp_stream_info_map.emplace("camera_1", 1);

    // init H264 config
    conf.bit_rate = 1024;
    conf.width = DEFAULT_WIDTH;
    conf.height = DEFAULT_HEIGHT;
    conf.gop_size = 60;
    conf.max_b_frames = 0;
    conf.frame_rate = 15;

    // get IP address of local machine
    try
    {
        rtsp_ip = ::getOwnIpV4Address();
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl << std::flush;
        return -1;
    }
    std::cout << "Successfully read IPv4 address.\n" << std::flush;

    // init OpenCV webcam video capture(s)
    for (const std::pair<const char*, uint16_t>& iterator : rtsp_stream_info_map)
    {
        webcam_capture.push_back(cv::VideoCapture());
        cv::VideoCapture& capture = webcam_capture.back();
#if defined(WIN32) || defined(_WIN32)
        if (!capture.open(iterator.second, cv::CAP_DSHOW))
#elif defined(__linux) || defined(__linux__)
        if (!capture.open(iterator.second))
#endif
        {
            capture.release();
            std::cerr << "Opening of OpenCV video capture(" << iterator.second << ") has been failed.\n" << std::flush;
            return -1;
        }
    }
    std::cout << "Successfully added OpenCV WebCam capture(s).\n" << std::flush;

    // init H264 Encoder(s)
    for (streaming_iterator = 0; streaming_iterator < rtsp_stream_info_map.size(); ++streaming_iterator)
    {
        AvH264Encoder* encoder = new AvH264Encoder;
        if (encoder->open(conf) < 0 || encoder->allocNewAvFrame() < 0)
        {
            encoder->close();
            delete encoder;
            std::cerr << "Failed to open AvH264Encoder." << std::endl;
            return -1;
        }
        h264_encoder.push_back(std::unique_ptr<AvH264Encoder>(encoder));
    }
    std::cout << "Successfully added FFMPEG H264 Encoder(s).\n" << std::flush;

    // init H264 Writer(s)
    for (const std::pair<const char*, uint16_t>& iterator : rtsp_stream_info_map)
    {
        std::string video_path = "";
        video_counter.push_back(0);
#ifdef OUTPUT_VIDEO_DIR
        video_path = OUTPUT_VIDEO_DIR;
        video_path += "/";
#endif
        video_path += "stream_" + std::to_string(iterator.second) + "_" + std::to_string(video_counter.back()++) + ".h264";
        h264_writer.push_back(std::make_unique<AvH264Writer>(video_path.c_str()));
        std::unique_ptr<AvH264Writer>& writer = h264_writer.back();
        try
        {
            writer->initWriter(conf);
        }
        catch (const std::runtime_error& e)
        {
            std::cerr << e.what() << std::endl << std::flush;
            return -1;
        }
    }
    std::cout << "Successfully initialized FFMPEG H264 Writer(s).\n" << std::flush;

    // start RTSP server
    if (!server->Start("0.0.0.0" /*INADDR_ANY*/, rtsp_port))
    {
        std::cerr << "RTSP Server listen on " << rtsp_port << " failed.\n" << std::flush;
        return -1;
    }
    std::cout << "Successfully started RTSP server.\n" << std::flush;

    // create RTSP streams
    for (const std::pair<const char*, uint16_t>& iterator : rtsp_stream_info_map)
    {
        xop::MediaSession* session = xop::MediaSession::CreateNew(iterator.first);
        std::string url = "rtsp://" + rtsp_ip + ":" + std::to_string(rtsp_port) + "/" + iterator.first;
        if (!session)
        {
            std::cerr << "Error: session for suffix = " << iterator.first << "is null.\n" << std::flush;
            return -1;
        }
        session->SetRtspUrl(url);
        rtsp_session.push_back(session);
    }

    // init RTSP streams
    for (xop::MediaSession* session : rtsp_session)
    {
        session->AddSource(xop::channel_0, xop::H264Source::CreateNew());
        if (!session->StartMulticast())
        {
            std::cerr << "Start Multicast for session " << session->GetRtspUrlSuffix() << " failed\n";
            return -1;
        }
        session->AddNotifyConnectedCallback([&](xop::MediaSessionId sessionId, std::string peer_ip, uint16_t peer_port)
            {
                printf("RTSP client connect, ip=%s, port=%hu, session %hu\n", peer_ip.c_str(), peer_port, sessionId);
            });
        session->AddNotifyDisconnectedCallback([&](xop::MediaSessionId sessionId, std::string peer_ip, uint16_t peer_port)
            {
                printf("RTSP client disconnect, ip=%s, port=%hu, session %hu\n", peer_ip.c_str(), peer_port, sessionId);
            });
        rtsp_session_id.push_back(server->AddSession(session));
    }
    std::cout << "Successfully created RTSP stream(s).\n" << std::flush;

    // init network manager
    manager = std::make_unique<NetworkManager>(manager_port, /*(uint16_t)rtsp_stream_info_map.size()*/2);
    for (xop::MediaSession* session : rtsp_session)
    {
        manager->appendStream(session->GetRtspUrl());
    }
    try
    {
        manager->initServer();
        manager->start();
    }
    catch (const std::exception& e)
    {
        std::cerr << "Exception on Network Manager init/start => " << e.what() << std::endl << std::flush;
        return -1;
    }
    std::cout << "Successfully started Network Manager.\n" << std::flush;

    
    // start streaming thread(s)
    for (streaming_iterator = 0; streaming_iterator < rtsp_stream_info_map.size(); ++streaming_iterator)
    {
        streaming_thread.push_back(std::thread(&::LiveStreamThread, &webcam_capture[streaming_iterator], h264_encoder[streaming_iterator].get(),
            h264_writer[streaming_iterator].get(), server.get(), rtsp_session_id[streaming_iterator], xop::channel_0, streaming_iterator));
    }

    net::Timer::Sleep(1000);
    std::cout << "Succesfully started streaming thread(s).\n\n" << std::flush;
    
    std::cin.get();
    std::cout << "====================================================\n";
    std::cout << "\tPRESS <ENTER> ONCE AGAIN TO EXIT!\n";
    std::cout << "====================================================\n" << std::flush;
    std::cin.get();

    mutex.lock();
    bExitThreads = true;
    mutex.unlock();

    // Stop frame sending thread(s)
    for (std::thread& thread : streaming_thread)
    {
        thread.join();
    }

    std::cout << "Successfull exit." << std::endl << std::flush;
    return 0;
}

static void LiveStreamThread(cv::VideoCapture* video_capture, AvH264Encoder* h264_encoder, AvH264Writer* h264_writer, xop::RtspServer* rtsp_server,
    xop::MediaSessionId session_id, xop::MediaChannelId channel_id, int link)
{
    static std::mutex mutex;
    AVPacket* av_packet = NULL;
    xop::AVFrame videoFrame;
    std::shared_ptr<xop::MediaSession> session = nullptr;
    cv::VideoCapture& capture = *video_capture;
    cv::Mat frame;

    session = rtsp_server->LookMediaSession(session_id);
    if (!session)
    {
        std::cerr << "Can't find session for session_id = " << session_id << std::endl << "Exit from thread.\n";
        return;
    }

    mutex.lock();
    std::cout << "\n=================================================================\n";
    std::cout << "session    : " << session->GetRtspUrlSuffix() << std::endl;
    std::cout << "mcast ip   : " << session->GetMulticastIp() << std::endl;
    std::cout << "mcast port : " << session->GetMulticastPort(xop::channel_0) << std::endl;
    std::cout << "Play URL   : " << session->GetRtspUrl() << std::endl;
    std::cout << "=================================================================\n";
    mutex.unlock();

    while (true)
    {
        {
            std::lock_guard<std::mutex> guard(mutex);
            if (bExitThreads)
            {
                break;
            }
        }
        capture >> frame;
        if (frame.data)
        {
            av_packet = h264_encoder->encode(frame);
            if (!av_packet)
            {
                continue;
            }
            videoFrame.timestamp = xop::H264Source::GetTimestamp();
            videoFrame.buffer = av_packet->data;
            videoFrame.size = av_packet->size;
            rtsp_server->PushFrame(session_id, channel_id, videoFrame);
            h264_writer->writeFrame(av_packet);
        }
        else
        {
            break;
        }
        net::Timer::Sleep(50);
    };
}
