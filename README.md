# MSc_RtspStreaming

#### DONE:
* Implemented Socket wrapper for TCP/UDP IP protocols
* Implemented RTSP Server console application with support of VLC Media Player streaming
* Implemented RTSP Client Qt GUI application
* Implemented Unicast RTP Client
* Implemented Multicast/"OnDemand" switching mode on RTP Client
* Implemented saving of cv::Mat camera capturing into H264 video on RTSP Server
* Implemented P2P (PeerToPeer) RTSP Server for sending "OnDemand" saved H264 videos
* Imported SQL Server C++ 3rd party library and created SQL Users Database
* Implemented Sign up/in of RTSP Client on SQL Database of RTSP Server
* Implemented Network Manager encryption of communication w/ Network User(s)

#### TO DO:
* Implement Sign out mechanism of RTSP Client
* Apply Code refactoring of RTSP Client
* Catch/Fix sprodacial crashes on RTSP Client
* Implement Web support - Web client
* Implement additional UI corrections on RTSP Client - i.e. Play/Pause/Stop buttons
* One account - one sign in limitation for RTSP Clients
* RTSP Pusher mechanism - Network manager (only RTSP Server) as medium between additional RTSP server and RTSP client
* Verify support for Linux
* Export apps as poratble binnaries w/ neccessary third-party dynamic libraries (DLL files)

##### NOTE: Potential improvement suggestions won't be considered because this is MSc student project
