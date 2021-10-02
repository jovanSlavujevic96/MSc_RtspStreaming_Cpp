#include <sstream>
#include <stdexcept>
#include <vector>
#include <random>

#include "NetworkUser.h"
#include "ctcp_client.h"

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QLineEdit>
#include <QMessageBox>

#define RESPOND_WORD_MAX_LEN 64u
#define RTSP_VERSION "RTSP/1.0"
#define RECURSION_NEWLINE "\r\n"
#define DOUBLE_RECURSION_NEWLINE "\r\n\r\n" 
#define POSITIVE_RESPONSE "OK"
#define POSITIVE_RESPONSE_CODE 200u
#define CONTENT_TYPE "application/sdp"
#define FRAME_WIDTH 640u
#define FRAME_HEIGHT 480u
#define NETWORK_MANAGER_PORT 9089u
#define LOCALHOST "127.0.0.1"

static inline const size_t cRecNwlLen = std::strlen(RECURSION_NEWLINE);

static const std::vector<const char*>& getResponseLines(const char* response) noexcept(false);
static bool parseRtspResponseFirstTwoLines(const char* line1, const char* line2, uint16_t cseqCtr);

MainWindow::MainWindow(QWidget* parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    mNetworkUserPort{NETWORK_MANAGER_PORT},
    mNetworkUserIp{LOCALHOST}
{
    ui->setupUi(this);

    rtsp_url = "";
    rtsp_ip = "";
    rtsp_suffix = "";
    rtsp_port = 0;
    rtsp_cseq_ctr = 0;
    rtsp_format = "";

    rtsp_client = NULL;

    QObject::connect(&mStreamingWorker, SIGNAL(dropFrame(cv::Mat)), this, SLOT(displayFrame(cv::Mat)));
    QObject::connect(&mStreamingWorker, SIGNAL(dropError(std::string, std::string)), this, SLOT(displayError(std::string, std::string)));
    QObject::connect(&mStreamingWorker, SIGNAL(dropWarning(std::string, std::string)), this, SLOT(displayWarning(std::string, std::string)));
    QObject::connect(&mStreamingWorker, SIGNAL(dropInfo(std::string, std::string)), this, SLOT(displayInfo(std::string, std::string)));

    QObject::connect(&mNetworkUserWorker, SIGNAL(dropLists(std::vector<std::string>, std::vector<std::string>)), this, SLOT(displayLists(std::vector<std::string>, std::vector<std::string>)));
    QObject::connect(&mNetworkUserWorker, SIGNAL(dropError(std::string, std::string)), this, SLOT(displayError(std::string, std::string)));
    QObject::connect(&mNetworkUserWorker, SIGNAL(dropWarning(std::string, std::string)), this, SLOT(displayWarning(std::string, std::string)));
    QObject::connect(&mNetworkUserWorker, SIGNAL(dropInfo(std::string, std::string)), this, SLOT(displayInfo(std::string, std::string)));

    MainWindow::stopRtspStream();
}

MainWindow::~MainWindow()
{
    mStreamingWorker.stop(false /*drop_info*/);
    mNetworkUserWorker.stop(false /*drop_info*/);
    if (rtsp_client)
    {
        delete rtsp_client;
    }
    delete ui;
}

void MainWindow::startRtspStream()
{
    if (mStreamingWorker.isRunning())
    {
        MainWindow::displayInfo("Information", "Streaming is already running");
        return;
    }
    try
    {
        MainWindow::parseRtspUrl();
    }
    catch (const std::runtime_error& e)
    {
        MainWindow::displayError("Parse RTSP URL Failed", e.what());
        return;
    }
    try
    {
        MainWindow::initRtspClient();
    }
    catch (const std::exception& e)
    {
        MainWindow::displayError("Bad Rtsp Client", e.what());
        MainWindow::stopRtspStream();
        return;
    }
    try
    {
        MainWindow::connectToRtspServer();
    }
    catch (const std::runtime_error& e)
    {
        MainWindow::displayError("Connect to RTSP server failed", e.what());
        MainWindow::stopRtspStream();
        rtsp_url = "NONE";
    }
    ui->runningStream_lineEdit->setText(rtsp_url.c_str());
}

void MainWindow::stopRtspStream()
{
    if (!mStreamingWorker.isRunning())
    {
        goto end_it;
    }
    try
    {
        MainWindow::rtspTeardown();
    }
    catch (const std::exception& e)
    {
        MainWindow::displayError("Teardown failed", e.what());
    }
    mStreamingWorker.stop();
end_it:
    if (rtsp_client)
    {
        delete rtsp_client;
        rtsp_client = NULL;
    }
    cv::Mat image = cv::Mat::zeros(FRAME_HEIGHT, FRAME_WIDTH, CV_8UC3);
    MainWindow::displayFrame(image);
    ui->runningStream_lineEdit->setText("NONE");
}

void MainWindow::displayFrame(cv::Mat frame_mat)
{
    qt_image = QImage(frame_mat.data, frame_mat.cols, frame_mat.rows, QImage::Format_BGR888);
    ui->label->setPixmap(QPixmap::fromImage(qt_image));
    ui->label->resize(ui->label->pixmap().size());
}

void MainWindow::displayLists(std::vector<std::string> streams, std::vector<std::string> urls)
{
    ui->rtspStreams_listWidget->clear();
    stream_to_url_map.clear();
    for (size_t i=0; i<streams.size(); i++)
    {
        stream_to_url_map.insert({ streams[i], urls[i] });
        ui->rtspStreams_listWidget->addItem(streams[i].c_str());
    }
}

void MainWindow::displayError(std::string title, std::string message)
{
    QMessageBox::critical(this, tr(title.c_str()), tr(message.c_str()));
}

void MainWindow::displayWarning(std::string title, std::string message)
{
    QMessageBox::warning(this, tr(title.c_str()), tr(message.c_str()));
}

void MainWindow::displayInfo(std::string title, std::string message)
{
    QMessageBox::information(this, tr(title.c_str()), tr(message.c_str()));
}

void MainWindow::on_pushButton_openStream_clicked()
{
    MainWindow::startRtspStream();
}

void MainWindow::on_pushButton_closeStream_clicked()
{
    MainWindow::stopRtspStream();
}

bool MainWindow::isMulticast(const char* ip) const
{
    uint32_t address;
    ::inet_pton(AF_INET, ip, &address);
    address = ::htonl(address);
    return ((address >= 0xE8000100) && (address <= 0xE8FFFFFF));
}

void MainWindow::parseRtspUrl() noexcept(false)
{
    const char* url = rtsp_url.c_str();
    uint16_t port = 0;
    char ip[RESPOND_WORD_MAX_LEN] = { 0 };
    char suffix[RESPOND_WORD_MAX_LEN] = { 0 };

    if ("" == rtsp_url)
    {
        throw std::runtime_error(
            "Can't parse RTSP URL.\n"\
            "There are no any RTSP URL selected.");
    }
    else if (std::strncmp(url, "rtsp://", 7) != 0)
    {
        throw std::runtime_error(
            "Bad format of RTSP URL.\n"\
            "URL must have rtsp:// at the beginning."
        );
    }

    // start parsing
    if (sscanf_s(url + 7, "%[^:]:%hu/%s", ip, RESPOND_WORD_MAX_LEN, &port, suffix, RESPOND_WORD_MAX_LEN) == 3)
    {

    }
    else if (sscanf_s(url + 7, "%[^/]/%s", ip, RESPOND_WORD_MAX_LEN, suffix, RESPOND_WORD_MAX_LEN) == 2)
    {
        port = 554;
    }
    else
    {
        throw std::runtime_error(
            "Bad format of RTSP URL.\n"\
            "URL must consists of server ip, port and channel suffix."
        );
    }
    rtsp_port = port;
    rtsp_ip = ip;
    rtsp_suffix = suffix;
}

void MainWindow::on_rtspStreams_listWidget_doubleClicked(const QModelIndex &index)
{
    std::string tmpStream = index.data(Qt::DisplayRole).toString().toStdString();
    std::string tmpUrl;

    for (std::pair<std::string, std::string> pair : stream_to_url_map)
    {
        if (pair.first == tmpStream)
        {
            tmpUrl = pair.second;
        }
    }

    if (mStreamingWorker.isRunning())
    {
        if (tmpUrl == rtsp_url)
        {
            MainWindow::displayInfo("Information", "Streaming is already running");
            return;
        }
        mStreamingWorker.stop();
    }
    rtsp_url = tmpUrl;
    MainWindow::stopRtspStream();
    MainWindow::startRtspStream();
}

void MainWindow::on_connectToManager_button_clicked()
{
    mNetworkUserWorker.setNetworkIp(mNetworkUserIp);
    mNetworkUserWorker.setNetworkPort(mNetworkUserPort);
    try
    {
        mNetworkUserWorker.start();
        displayInfo("Connect To Manager", "Successfully connected to manager");
    }
    catch (const CSocketException& e)
    {
        displayError("Error on Network User connection", e.what());
    }
    catch (const std::exception& e)
    {
        displayError("Error on Network User startup", e.what());
    }
}

void MainWindow::on_lineEdit_textEdited(const QString &arg1)
{
    mNetworkUserIp = arg1.toStdString();
}

void MainWindow::on_lineEdit_returnPressed()
{
    MainWindow::on_connectToManager_button_clicked();
}

void MainWindow::initRtspClient() noexcept(false)
{
    if (!rtsp_client)
    {
        if (rtsp_ip == "" || !rtsp_port || rtsp_suffix == "")
        {
            std::stringstream ss;
            ss << "RTSP IP = " << rtsp_ip << " && PORT = " << rtsp_port << " && SUFFIX = " << rtsp_suffix;
            throw std::runtime_error(ss.str());
        }
        rtsp_client = new CTcpClient(rtsp_ip.c_str(), rtsp_port);
    }
    rtsp_client->initClient();
}

void MainWindow::connectToRtspServer() noexcept(false)
{
    MainWindow::rtspOptions();        //throws errors
    MainWindow::rtspDescribe();       //throws errors
    MainWindow::rtspSetup();          //throws errors
    mStreamingWorker.start();         //throws errors
    MainWindow::rtspPlay();           //throws errors
}

void MainWindow::rtspOptions() noexcept(false)
{
    CTcpClient& clientRtsp = *rtsp_client;
    std::string sendingMessage;
    {
        std::stringstream ss;
        ss << MethodToString[static_cast<size_t>(RtspMethod::OPTIONS)] << ' ' << rtsp_url << ' ' << RTSP_VERSION << RECURSION_NEWLINE;
        ss << "CSeq: " << ++rtsp_cseq_ctr << RECURSION_NEWLINE;
        ss << "User - Agent: RtspClient / v1.0.0 (by Jovan Slavujevic)" << DOUBLE_RECURSION_NEWLINE;
        sendingMessage = ss.str();
    }
    clientRtsp << sendingMessage;
    rtsp_message.clear();
    uint16_t ret = (uint16_t)(clientRtsp >> &rtsp_message);
    rtsp_message.setCurrentSize(ret);
    const std::vector<const char*>& responseLine = ::getResponseLines(rtsp_message.cData());
    if (!::parseRtspResponseFirstTwoLines(rtsp_message.cData(), responseLine[0], rtsp_cseq_ctr) )
    {
        throw std::runtime_error("Bad format of 1st/2nd line of RTSP OPTIONS response.");
    }
    const char* Methods = std::strstr(responseLine[1], "Public: ");
    if (!Methods)
    {
        throw std::runtime_error("Bad format of 3rd line of RTSP OPTIONS response.");
    }
}

void MainWindow::rtspDescribe() noexcept(false)
{
    IClient& clientRtsp = *rtsp_client;
    std::string sendingMessage;
    {
        std::stringstream ss;
        ss << MethodToString[static_cast<size_t>(RtspMethod::DESCRIBE)] << ' ' << rtsp_url << ' ' << RTSP_VERSION << RECURSION_NEWLINE;
        ss << "CSeq: " << ++rtsp_cseq_ctr << RECURSION_NEWLINE;
        ss << "User - Agent: RtspClient / v1.0.0 (by Jovan Slavujevic)" << RECURSION_NEWLINE;
        ss << "Accept: " << CONTENT_TYPE << DOUBLE_RECURSION_NEWLINE;
        sendingMessage = ss.str();
    }
    clientRtsp << sendingMessage;
    rtsp_message.clear();
    uint16_t ret = (uint16_t)(clientRtsp >> &rtsp_message);
    rtsp_message.setCurrentSize(ret);
    const std::vector<const char*>& responseLine = ::getResponseLines(rtsp_message.cData());
    if (!::parseRtspResponseFirstTwoLines(rtsp_message.cData(), responseLine[0], rtsp_cseq_ctr))
    {
        throw std::runtime_error("Bad format of 1st/2nd line of RTSP DESCRIBE response.");
    }
    int scanf_ret;
    int32_t contentLength;
    char response_word[4][RESPOND_WORD_MAX_LEN] = { {0} };
    scanf_ret = sscanf_s(responseLine[1], "%s %d", response_word[0], RESPOND_WORD_MAX_LEN, &contentLength);
    if (scanf_ret != 2 || std::strcmp(response_word[0], "Content-Length:") || contentLength <= 0)
    {
        throw std::runtime_error("Bad format of 3rd line (Content-Length) of RTSP DESCRIBE response.");
    }
    scanf_ret = sscanf_s(responseLine[2], "%s %s", response_word[0], RESPOND_WORD_MAX_LEN, response_word[1], RESPOND_WORD_MAX_LEN);
    if (scanf_ret != 2 || std::strcmp(response_word[0], "Content-Type:") || std::strcmp(response_word[1], CONTENT_TYPE))
    {
        throw std::runtime_error("Bad format of 4th line (Content-Type) of RTSP DESCRIBE response.");
    }
    const char* SDP = responseLine[3] + ::cRecNwlLen;
    if (std::strlen(SDP) < (size_t)contentLength)
    {
        throw std::runtime_error("Bad SDP format of RTSP DESCRIBE response.");
    }
    const char* videoLine = std::strstr(SDP, "m=video");
    if (!videoLine)
    {
        throw std::runtime_error("Bad RTSP DESCRIBE response -> Missing video line.");
    }
    scanf_ret = sscanf_s(videoLine, "%s %hu %s %s\r\n", response_word[0], RESPOND_WORD_MAX_LEN, &ret, response_word[1], RESPOND_WORD_MAX_LEN, response_word[2], RESPOND_WORD_MAX_LEN);
    if (scanf_ret != 4)
    {
        throw std::runtime_error("Bad RTSP DESCRIBE response -> Bad format of video line.");
    }
    if (ret)
    {
        mStreamingWorker.setRtpServerPort(ret);
    }
    else
    {
        std::random_device rd;
        while (!ret)
        {
            ret = rd() % 0xFFFFu;
        }
        mStreamingWorker.setRtpClientPort(ret);
    }
    rtsp_format = response_word[1]; // RTP/AVP
    const char* networkLine = std::strstr(SDP, "c=IN");
    const char* ip;
    if (networkLine)
    {
        scanf_ret = sscanf_s(networkLine, "%s %s %[^/]/%hu\r\n", response_word[0], RESPOND_WORD_MAX_LEN, response_word[1], RESPOND_WORD_MAX_LEN, response_word[2], RESPOND_WORD_MAX_LEN, &ret);
        if (scanf_ret != 4 || strcmp(response_word[1], "IP4"))
        {
            throw std::runtime_error("Bad RTSP DESCRIBE response -> Bad format of network line.");
        }
        ip = response_word[2];
    }
    else
    {
        ip = rtsp_ip.c_str();
    }
    mStreamingWorker.setRtpServerIp(ip);
    mStreamingWorker.IsRtpMulticast(MainWindow::isMulticast(ip));
}

void MainWindow::rtspSetup() noexcept(false)
{
    IClient& clientRtsp = *rtsp_client;
    std::string sendingMessage;
    {
        std::stringstream ss;
        const char* casting;
        uint16_t port;
        if (mStreamingWorker.IsRtpMulticast())
        {
            casting = "multicast";
            port = mStreamingWorker.getRtpServerPort();
        }
        else
        {
            casting = "unicast";
            port = mStreamingWorker.getRtpClientPort();
        }
        ss << MethodToString[static_cast<size_t>(RtspMethod::SETUP)] << ' ' << rtsp_url << ' ' << RTSP_VERSION << RECURSION_NEWLINE;
        ss << "CSeq: " << ++rtsp_cseq_ctr << RECURSION_NEWLINE;
        ss << "User - Agent: RtspClient / v1.0.0 (by Jovan Slavujevic)" << RECURSION_NEWLINE;
        ss << "Transport: " << rtsp_format << ';' << casting << ";port=" << port << "-0" << DOUBLE_RECURSION_NEWLINE;
        sendingMessage = ss.str();
    }
    clientRtsp << sendingMessage;
    rtsp_message.clear();
    uint16_t rcvBytes;
    uint16_t ret[2] = { 0 };
    rcvBytes = (uint16_t)(clientRtsp >> &rtsp_message);
    rtsp_message.setCurrentSize(rcvBytes);
    const std::vector<const char*>& responseLine = ::getResponseLines(rtsp_message.cData());
    if (!::parseRtspResponseFirstTwoLines(rtsp_message.cData(), responseLine[0], rtsp_cseq_ctr))
    {
        throw std::runtime_error("Bad format of 1st/2nd line of RTSP SETUP response.");
    }
    int scanf_ret, scanf_ret2;
    char response_word[7][RESPOND_WORD_MAX_LEN] = { {0} };
    scanf_ret = sscanf_s(responseLine[1], "%s %[^;];%[^;];%[^;];%[^;];%[^;];%s",
        response_word[0], RESPOND_WORD_MAX_LEN,
        response_word[1], RESPOND_WORD_MAX_LEN,
        response_word[2], RESPOND_WORD_MAX_LEN,
        response_word[3], RESPOND_WORD_MAX_LEN,
        response_word[4], RESPOND_WORD_MAX_LEN,
        response_word[5], RESPOND_WORD_MAX_LEN,
        response_word[6], RESPOND_WORD_MAX_LEN
    );
    scanf_ret2 = sscanf_s(responseLine[1], "%s %[^;];%[^;];%[^;];%s",
        response_word[0], RESPOND_WORD_MAX_LEN,
        response_word[1], RESPOND_WORD_MAX_LEN,
        response_word[2], RESPOND_WORD_MAX_LEN,
        response_word[3], RESPOND_WORD_MAX_LEN,
        response_word[4], RESPOND_WORD_MAX_LEN
    );
    if ( (scanf_ret != 7 && scanf_ret2 != 5) || strcmp(response_word[0], "Transport:") || (rtsp_format != response_word[1]) )
    {
        std::stringstream ss;
        ss << "Bad format of 3rd line of RTSP SETUP response.\n\n" << std::string( responseLine[1], std::strstr(responseLine[1], RECURSION_NEWLINE) );
        throw std::runtime_error(ss.str());
    }
    if (7 == scanf_ret)
    {
        // multicast RTP
        mStreamingWorker.IsRtpMulticast(true);
        scanf_ret = sscanf_s(response_word[3], "%[^=]=%s", response_word[0], RESPOND_WORD_MAX_LEN, response_word[1], RESPOND_WORD_MAX_LEN);
        if (scanf_ret != 2 || std::strcmp(response_word[0], "destination"))
        {
            throw std::runtime_error("Bad format of destination line of RTSP SETUP response.");
        }
        if (sscanf_s(response_word[5], "%[^=]=%hu-%hu", response_word[0], RESPOND_WORD_MAX_LEN, ret, ret + 1) != 3 || strcmp(response_word[0], "port") || ret[1] != 0)
        {
            throw std::runtime_error("Bad format of port line of RTSP SETUP response.");
        }
        mStreamingWorker.setRtpServerIp(response_word[1]);
        mStreamingWorker.setRtpServerPort(ret[0]);
    }
    else if (5 == scanf_ret2)
    {
        // unicast RTP
        mStreamingWorker.IsRtpMulticast(false);
        mStreamingWorker.setRtpServerIp(rtsp_ip.c_str());
        if (sscanf_s(response_word[3], "%[^=]=%hu-%hu", response_word[0], RESPOND_WORD_MAX_LEN, ret, ret + 1) != 3 || strcmp(response_word[0], "client_port"))
        {
            throw std::runtime_error("Bad format of port line of RTSP SETUP response.");
        }
        mStreamingWorker.setRtpClientPort(ret[0]);
        if (sscanf_s(response_word[4], "%[^=]=%hu-%hu", response_word[0], RESPOND_WORD_MAX_LEN, ret, ret + 1) != 3 || strcmp(response_word[0], "server_port"))
        {
            throw std::runtime_error("Bad format of port line of RTSP SETUP response.");
        }
        mStreamingWorker.setRtpServerPort(ret[0]);
    }
    scanf_ret = sscanf_s(responseLine[2], "%s %hu", response_word[0], RESPOND_WORD_MAX_LEN, ret);
    if (scanf_ret != 2)
    {
        throw std::runtime_error("Bad format of 4th line of RTSP SETUP response.");
    }
    rtsp_session = ret[0];
}

void MainWindow::rtspPlay() noexcept(false)
{
    IClient& clientRtsp = *rtsp_client;
    std::string sendingMessage;
    {
        std::stringstream ss;
        ss << MethodToString[static_cast<size_t>(RtspMethod::PLAY)] << ' ' << rtsp_url << ' ' << RTSP_VERSION << RECURSION_NEWLINE;
        ss << "CSeq: " << ++rtsp_cseq_ctr << RECURSION_NEWLINE;
        ss << "User - Agent: RtspClient / v1.0.0 (by Jovan Slavujevic)" << RECURSION_NEWLINE;
        ss << "Session: " << rtsp_session << RECURSION_NEWLINE;
        ss << "Range: npt=0.000-" << DOUBLE_RECURSION_NEWLINE;
        sendingMessage = ss.str();
    }
    clientRtsp << sendingMessage;
    rtsp_message.clear();
    uint16_t ret = (uint16_t)(clientRtsp >> &rtsp_message);
    rtsp_message.setCurrentSize(ret);
    const std::vector<const char*>& responseLine = ::getResponseLines(rtsp_message.cData());
    if (!::parseRtspResponseFirstTwoLines(rtsp_message.cData(), responseLine[0], rtsp_cseq_ctr))
    {
        throw std::runtime_error("Bad format of 1st/2nd line of RTSP DESCRIBE response.");
    }
}

void MainWindow::rtspTeardown() noexcept(false)
{
    if (!rtsp_client)
    {
        return;
    }
    IClient& clientRtsp = *rtsp_client;
    std::string sendingMessage;
    {
        std::stringstream ss;
        ss << MethodToString[static_cast<size_t>(RtspMethod::TEARDOWN)] << ' ' << rtsp_url << ' ' << RTSP_VERSION << RECURSION_NEWLINE;
        ss << "CSeq: " << ++rtsp_cseq_ctr << RECURSION_NEWLINE;
        ss << "User - Agent: RtspClient / v1.0.0 (by Jovan Slavujevic)" << RECURSION_NEWLINE;
        ss << "Session: " << rtsp_session << DOUBLE_RECURSION_NEWLINE;
        sendingMessage = ss.str();
    }
    clientRtsp << sendingMessage;
    rtsp_message.clear();
    uint16_t ret = (uint16_t)(clientRtsp >> &rtsp_message);
    rtsp_message.setCurrentSize(ret);
    const std::vector<const char*>& responseLine = ::getResponseLines(rtsp_message.cData());
    if (!::parseRtspResponseFirstTwoLines(rtsp_message.cData(), responseLine[0], rtsp_cseq_ctr))
    {
        throw std::runtime_error("Bad format of 1st/2nd line of RTSP DESCRIBE response.");
    }
}

static const std::vector<const char*>& getResponseLines(const char* response) noexcept(false)
{
    static std::vector<const char*> responseLines;
    responseLines.clear();
    const char* singleRecursiveNwln = std::strstr(response, RECURSION_NEWLINE);
    const char* doubleRecursiveNwln = std::strstr(response, DOUBLE_RECURSION_NEWLINE);
    if (!doubleRecursiveNwln || !singleRecursiveNwln || singleRecursiveNwln == doubleRecursiveNwln)
    {
        throw std::runtime_error("Bad response format!");
    }
    while (singleRecursiveNwln != doubleRecursiveNwln)
    {
        responseLines.push_back(singleRecursiveNwln + ::cRecNwlLen);
        singleRecursiveNwln = std::strstr(singleRecursiveNwln + ::cRecNwlLen, RECURSION_NEWLINE);
    }
    responseLines.push_back(doubleRecursiveNwln + ::cRecNwlLen);
    return responseLines;
}

static bool parseRtspResponseFirstTwoLines(const char* line1, const char* line2, uint16_t cseqCtr)
{
    int scanf_ret;
    uint16_t response_nr;
    char response_word[2][RESPOND_WORD_MAX_LEN] = { {0} };
    scanf_ret = sscanf_s(line1, "%s %hu %s" RECURSION_NEWLINE, response_word[0], RESPOND_WORD_MAX_LEN, &response_nr, response_word[1], RESPOND_WORD_MAX_LEN);
    if (scanf_ret != 3 || std::strcmp(response_word[0], RTSP_VERSION) || POSITIVE_RESPONSE_CODE != response_nr || std::strcmp(response_word[1], POSITIVE_RESPONSE))
    {
        // Bad 1st line
        return false;
    }
    scanf_ret = sscanf_s(line2, "%s %hu" RECURSION_NEWLINE, response_word[0], RESPOND_WORD_MAX_LEN, &response_nr);
    if (scanf_ret != 2 || response_nr != cseqCtr)
    {
        // Bad 2nd line
        return false;
    }
    return true;
}
