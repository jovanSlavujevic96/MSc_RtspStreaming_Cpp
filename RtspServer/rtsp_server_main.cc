#include <cstdint>
#include <string>
#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <map>
#include <mutex>
#include <atomic>
#include <condition_variable>

#include "xop/RtspServer.h"
#include "xop/H264Source.h"
#include "xop/rtp.h"

#include "H264StreamingAPI/AvH264Encoder.h"
#include "H264StreamingAPI/AvH264Writer.h"
#include "H264StreamingAPI/Timestamp.h"
#include "NetworkManagerAPI/NetworkManager.h"
#include "SocketNetworking/socket_net/include/socket_utils.h"

#include "OnDemand/OnDemandSession.h"

#define DEFAULT_WIDTH 640u
#define DEFAULT_HEIGHT 480u

#define DEFAULT_NETWORK_MANAGER_PORT 9089u
#define DEFAULT_RTSP_SERVER_PORT 9090u
#define NUMBER_OF_FRAMES_IN_VIDEO 2045u

static std::atomic<bool> bExitThreads = false;
static std::mutex mutex;

static void LiveStreamThread(cv::VideoCapture* video_capture, AvH264Encoder* h264_encoder, AvH264Writer* h264_writer,
    std::shared_ptr<xop::RtspServer> rtsp_server, xop::MediaSessionId session_id, xop::MediaChannelId channel_id);
static void OnDemandStreamThread(AvH264Writer* h264_writer, std::shared_ptr<xop::RtspServer> rtsp_server, NetworkManager* manager,
    xop::MediaSessionId session_id, xop::MediaChannelId channel_id, std::shared_ptr<std::condition_variable> condition);

struct RtspStreamInfo
{
    int camera_id;
    std::string stream_suffix;
    bool multicast;
};

int __cdecl main()
{
    // RTSP info variables
    constexpr uint16_t rtsp_port = DEFAULT_RTSP_SERVER_PORT;
    std::string rtsp_ip;
    std::vector<RtspStreamInfo> rtsp_stream_info;

    // Video processing variables
    AvH264EncConfig conf;
    std::vector<AvH264Encoder> h264_encoder;
    std::vector<cv::VideoCapture> webcam_capture;
    std::vector<AvH264Writer> h264_writer;

    // RTSP/RTP handler variables
    net::EventLoop event_loop;
    std::shared_ptr<xop::RtspServer>server = xop::RtspServer::Create(&event_loop);
    std::vector<xop::MediaSessionId> rtsp_session_id;
    std::vector<xop::MediaSession*> rtsp_session;
    std::vector<std::shared_ptr<std::condition_variable>> streaming_notifier;
    std::vector<std::thread> streaming_thread;
    uint16_t streaming_iterator;

    // Network Manager variables/constants
    constexpr uint16_t manager_port = DEFAULT_NETWORK_MANAGER_PORT;
    std::unique_ptr<NetworkManager> manager = nullptr;

    // init list of streams
    rtsp_stream_info.push_back({ 0, "camera_0", true });
    rtsp_stream_info.push_back({ 1, "camera_1", true });

    // init H264 config
    conf.bit_rate = 1024;
    conf.width = DEFAULT_WIDTH;
    conf.height = DEFAULT_HEIGHT;
    conf.gop_size = 60;
    conf.max_b_frames = 0;
    conf.frame_rate = 15;

    // disable av logs
    av_log_set_level(0);

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
    for (const RtspStreamInfo& stream_info : rtsp_stream_info)
    {
        webcam_capture.push_back(cv::VideoCapture());
        cv::VideoCapture& capture = webcam_capture.back();
#if defined(WIN32) || defined(_WIN32)
        if (!capture.open(stream_info.camera_id, cv::CAP_DSHOW))
#elif defined(__linux) || defined(__linux__)
        if (!capture.open(stream_info.camera_id))
#endif
        {
            capture.release();
            std::cerr << "Opening of OpenCV video capture(" << stream_info.camera_id << ") has been failed.\n" << std::flush;
            return -1;
        }
    }
    std::cout << "Successfully added OpenCV WebCam capture(s).\n" << std::flush;

    // init H264 Encoder(s)
    for (streaming_iterator = 0; streaming_iterator < rtsp_stream_info.size(); ++streaming_iterator)
    {
        h264_encoder.push_back(AvH264Encoder());
        AvH264Encoder* encoder = &h264_encoder.back();
        if (encoder->open(conf) < 0 || encoder->allocNewAvFrame() < 0)
        {
            encoder->close();
            std::cerr << "Failed to open AvH264Encoder." << std::endl << std::flush;
            return -1;
        }
    }
    std::cout << "Successfully added FFMPEG H264 Encoder(s).\n" << std::flush;

    // init H264 Writer(s)
    for (const RtspStreamInfo& stream_info : rtsp_stream_info)
    {
        const char* path = NULL;
#ifdef OUTPUT_VIDEO_DIR
        path = OUTPUT_VIDEO_DIR;
#endif
        h264_writer.push_back(AvH264Writer(path, stream_info.camera_id, 1u));
        AvH264Writer* writer = &h264_writer.back();
        writer->bindEncodingConfiguration(&conf);
        streaming_notifier.push_back(std::make_shared<std::condition_variable>());
        writer->bindStreamingNotifier(streaming_notifier.back());
    }
    std::cout << "Successfully initialized FFMPEG H264 Writer(s).\n" << std::flush;

    // start RTSP server
    if (!server->Start("0.0.0.0" /*INADDR_ANY*/, rtsp_port))
    {
        std::cerr << "RTSP Server listen on " << rtsp_port << " failed.\n" << std::flush;
        return -1;
    }
    std::cout << "Successfully started RTSP server.\n" << std::flush;

    // update list of streams
    rtsp_stream_info.push_back({ -1, "on_demand_camera_0", false });
    rtsp_stream_info.push_back({ -1, "on_demand_camera_1", false });

    // create RTSP Live stream(s)
    streaming_iterator = 0;
    for (const RtspStreamInfo& stream_info : rtsp_stream_info)
    {
        xop::MediaSession* session;
        if (!std::strstr(stream_info.stream_suffix.c_str(), "on_demand"))
        {
            session = xop::MediaSession::CreateNew(stream_info.stream_suffix);
        }
        else
        {
            OnDemandSession* demand_session = new OnDemandSession(stream_info.stream_suffix);
            demand_session->bindH264Writer(&h264_writer[streaming_iterator]);
            session = demand_session;
            streaming_iterator = (streaming_iterator + 1) % h264_writer.size();
        }
        std::string url = "rtsp://" + rtsp_ip + ":" + std::to_string(rtsp_port) + "/" + stream_info.stream_suffix;
        if (!session)
        {
            std::cerr << "Error: session for suffix = " << stream_info.stream_suffix << "is null.\n" << std::flush;
            return -1;
        }
        session->SetRtspUrl(url);
        session->AddSource(xop::channel_0, xop::H264Source::CreateNew());
        if (stream_info.multicast)
        {
            if (!session->StartMulticast())
            {
                std::cerr << "Start Multicast for session " << session->GetRtspUrlSuffix() << " failed\n";
                return -1;
            }
        }
        session->AddNotifyConnectedCallback([&](xop::MediaSessionId sessionId, std::string peer_ip, uint16_t peer_port)
            {
                printf("RTSP client connect, ip=%s, port=%hu, session %hu\n", peer_ip.c_str(), peer_port, sessionId);
            });
        session->AddNotifyDisconnectedCallback([&](xop::MediaSessionId sessionId, std::string peer_ip, uint16_t peer_port)
            {
                printf("RTSP client disconnect, ip=%s, port=%hu, session %hu\n", peer_ip.c_str(), peer_port, sessionId);
            });
        rtsp_session.push_back(session);
        rtsp_session_id.push_back(server->AddSession(session));
    }
    std::cout << "Successfully created RTSP stream(s).\n" << std::flush;

    // init network manager
    manager = std::make_unique<NetworkManager>(manager_port);
    for (xop::MediaSession* session : rtsp_session)
    {
        if (dynamic_cast<OnDemandSession*>(session))
        {
            continue;
        }
        manager->updateLiveStream(session->GetRtspUrlSuffix(), session->GetRtspUrl());
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
    for (streaming_iterator = 0; streaming_iterator < rtsp_stream_info.size(); ++streaming_iterator)
    {
        const RtspStreamInfo& stream_info = rtsp_stream_info[streaming_iterator];
        if (std::strstr(stream_info.stream_suffix.c_str(), "on_demand"))
        {
            uint16_t iterator = streaming_iterator % (rtsp_stream_info.size() / 2);
            streaming_thread.push_back(std::thread(&::OnDemandStreamThread, &h264_writer[iterator], server, manager.get(),
                rtsp_session_id[streaming_iterator], xop::channel_0, streaming_notifier[iterator]));
        }
        else
        {
            streaming_thread.push_back(std::thread(&::LiveStreamThread, &webcam_capture[streaming_iterator], &h264_encoder[streaming_iterator],
                &h264_writer[streaming_iterator], server, rtsp_session_id[streaming_iterator], xop::channel_0));
        }
    }

    net::Timer::Sleep(1000);
    std::cout << "Succesfully started streaming thread(s).\n\n" << std::flush;
    
    std::cin.get();
    std::cout << "====================================================\n";
    std::cout << "\tPRESS <ENTER> ONCE AGAIN TO EXIT!\n";
    std::cout << "====================================================\n" << std::flush;
    std::cin.get();

    // Notify streaming threads to exit
    mutex.lock();
    bExitThreads = true;
    mutex.unlock();

    // Notify blocked OnDemand streaming threads to exit
    for (std::shared_ptr<std::condition_variable>& condition : streaming_notifier)
    {
        condition->notify_one();
    }

    // Stop frame sending thread(s)
    for (std::thread& thread : streaming_thread)
    {
        thread.join();
    }

    std::cout << "Successfull exit." << std::endl << std::flush;
    return 0;
}

static void LiveStreamThread(cv::VideoCapture* video_capture, AvH264Encoder* h264_encoder, AvH264Writer* h264_writer,
    std::shared_ptr<xop::RtspServer> rtsp_server, xop::MediaSessionId session_id, xop::MediaChannelId channel_id)
{
    AVPacket* av_packet = NULL;
    xop::AVFrame video_frame;
    std::shared_ptr<xop::MediaSession> session = nullptr;
    cv::VideoCapture& capture = *video_capture;
    cv::Mat frame;
    uint32_t frame_counter = 0;

    session = rtsp_server->LookMediaSession(session_id);
    if (!session)
    {
        std::cerr << "LiveStreamThread : Can't find session for session_id = " << session_id << std::endl << "Exit from thread.\n" << std::flush;
        return;
    }

    mutex.lock();
    std::cout << "\n=================================================================\n";
    std::cout << "session    : " << session->GetRtspUrlSuffix() << std::endl;
    std::cout << "mcast ip   : " << session->GetMulticastIp() << std::endl;
    std::cout << "mcast port : " << session->GetMulticastPort(xop::channel_0) << std::endl;
    std::cout << "Play URL   : " << session->GetRtspUrl() << std::endl;
    std::cout << "=================================================================\n" << std::flush;
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
        try
        {
            if (0 == frame_counter)
            {
                h264_writer->initWriter();
            }
        }
        catch (const std::exception& e)
        {
            std::cerr << "LiveStreamThread : " << e.what() << std::endl << std::flush;
            break;
        }
        capture >> frame;
        if (frame.empty())
        {
            break;
        }
        cv::putText(frame, ::getLongTimestampStr(), cv::Point(30, 30), cv::FONT_HERSHEY_COMPLEX_SMALL, 0.8, cv::Scalar(200, 200, 250), 1, cv::LINE_AA);
        av_packet = h264_encoder->encode(frame);
        if (!av_packet)
        {
            continue;
        }
        video_frame.timestamp = xop::H264Source::GetTimestamp();
        video_frame.buffer = av_packet->data;
        video_frame.size = av_packet->size;
        rtsp_server->PushFrame(session_id, channel_id, video_frame);
        h264_writer->writeFrame(av_packet);
        frame_counter++;
        try
        {
            if (NUMBER_OF_FRAMES_IN_VIDEO == frame_counter)
            {
                h264_writer->closeWriter();
                frame_counter = 0;
            }
        }
        catch (const std::exception& e)
        {
            std::cerr << "LiveStreamThread : " << e.what() << std::endl << std::flush;
            break;
        }
        net::Timer::Sleep(40);
    };
}

static void OnDemandStreamThread(AvH264Writer* h264_writer, std::shared_ptr<xop::RtspServer> rtsp_server, NetworkManager* manager,
    xop::MediaSessionId session_id, xop::MediaChannelId channel_id, std::shared_ptr<std::condition_variable> condition)
{
    std::string current_recording_file;
    const int buf_size = 2000000;
    xop::AVFrame videoFrame(buf_size);
    bool end_of_file = true;
    bool wait_for_file = false;
    std::shared_ptr<xop::MediaSession> session = nullptr;

    std::mutex on_demand_mutex;
    std::unique_lock<std::mutex> unique_lock(on_demand_mutex);
    condition->wait(unique_lock);
    if (bExitThreads)
    {
        return;
    }

    session = rtsp_server->LookMediaSession(session_id);
    if (!session)
    {
        std::cerr << "OnDemandStreamThread : Can't find session for session_id = " << session_id << std::endl << "Exit from thread.\n" << std::flush;
        return;
    }

    mutex.lock();
    std::cout << "\n=================================================================\n";
    std::cout << "session    : " << session->GetRtspUrlSuffix() << std::endl;
    std::cout << "Play URL   : " << session->GetRtspUrl() << std::endl;
    std::cout << "=================================================================\n" << std::flush;

    manager->updateOnDemandStream(session->GetRtspUrlSuffix(), session->GetRtspUrl(), false /*is_busy*/);
    mutex.unlock();

    // TO DO : Move this from here.
}
