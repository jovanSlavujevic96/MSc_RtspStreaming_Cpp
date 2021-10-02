#include "xop/RtspServer.h"
#include "EnterpriseEmitter/EnterpriseEmitter.h"
#include "SocketNetworking/socket_net/include/socket_utils.h"

#define DEFAULT_WIDTH 640u
#define DEFAULT_HEIGHT 480u

#define DEFAULT_NETWORK_MANAGER_PORT 9089u
#define DEFAULT_RTSP_SERVER_PORT 9090u
#define NUMBER_OF_FRAMES_IN_VIDEO 2045u

int __cdecl main()
{
    // RTSP Server variables/constants
    constexpr uint16_t rtsp_port = DEFAULT_RTSP_SERVER_PORT;
    std::string rtsp_ip;
    std::vector<std::string> rtsp_streams;
    net::EventLoop event_loop;
    std::shared_ptr<xop::RtspServer>server = xop::RtspServer::Create(&event_loop);

    // Network Manager variables/constants
    constexpr uint16_t manager_port = DEFAULT_NETWORK_MANAGER_PORT;
    std::shared_ptr<NetworkManager> manager = nullptr;

    // Video processing variables
    std::shared_ptr<AvH264EncConfig> h264_configuation(new AvH264EncConfig);

    // RTSP Enterprise Emitter(s)
    std::vector<std::shared_ptr<EnterpriseEmitter>> rtsp_emitter;

    // init list of streams
    rtsp_streams.push_back("camera_0");
    rtsp_streams.push_back("camera_1");

    // init H264 config
    h264_configuation->bit_rate = 1024;
    h264_configuation->width = DEFAULT_WIDTH;
    h264_configuation->height = DEFAULT_HEIGHT;
    h264_configuation->gop_size = 60;
    h264_configuation->max_b_frames = 0;
    h264_configuation->frame_rate = 15;

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

    // start RTSP server
    if (!server->Start("0.0.0.0" /*INADDR_ANY*/, rtsp_port))
    {
        std::cerr << "RTSP Server listen on " << rtsp_port << " failed.\n" << std::flush;
        return -1;
    }
    std::cout << "Successfully started RTSP server.\n" << std::flush;

    // init network manager
    manager = std::make_shared<NetworkManager>(manager_port);
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

    // create emitter(s)
    for (uint8_t i = 0; i < rtsp_streams.size(); ++i)
    {
        try
        {
            rtsp_emitter.push_back(
                EnterpriseEmitter::createNew(server, manager, i, h264_configuation, OUTPUT_VIDEO_DIR, rtsp_streams[i], rtsp_ip, rtsp_port)
            );
        }
        catch (const std::exception& e)
        {
            std::cerr << "Exception on => " << e.what() << std::endl << std::flush;
            return -1;
        }
    }

    for (std::shared_ptr<EnterpriseEmitter> emitter : rtsp_emitter)
    {
        emitter->start();
    }

    net::Timer::Sleep(1000);
    std::cout << "Succesfully started streaming thread(s).\n\n" << std::flush;
    
    std::cin.get();
    std::cout << "====================================================\n";
    std::cout << "\tPRESS <ENTER> ONCE AGAIN TO EXIT!\n";
    std::cout << "====================================================\n" << std::flush;
    std::cin.get();

    for (std::shared_ptr<EnterpriseEmitter> emitter : rtsp_emitter)
    {
        emitter->stop();
    }

    std::cout << "Successfull exit." << std::endl << std::flush;
    return 0;
}
