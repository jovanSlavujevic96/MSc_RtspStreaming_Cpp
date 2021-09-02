#include <sstream>
#include <stdexcept>
#include <vector>

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
#define DEFAULT_RTSP_URL "rtsp://127.0.0.1:9090/live"

static inline const size_t cRecNwlLen = std::strlen(RECURSION_NEWLINE);

static const std::vector<const char*>& getResponseLines(const char* response) noexcept(false);
static bool parseRtspResponmseFirstTwoLines(const char* line1, const char* line2, uint16_t cseqCtr);

MainWindow::MainWindow(QWidget* parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    rtsp_url = DEFAULT_RTSP_URL;
    rtsp_ip = "";
    rtsp_suffix = "";
    rtsp_port = 0;
    rtsp_cseq_ctr = 0;
    rtsp_format = "";

    rtp_mcast_ip = "";
    rtp_mcast_port = 0;

    QObject::connect(&mStreamingWorker, SIGNAL(updateWindow(const cv::Mat&)), this, SLOT(updateScreen(const cv::Mat&)));
    QObject::connect(&mStreamingWorker, SIGNAL(dropError(const char*, const char*)), this, SLOT(displayError(const char*, const char*)));
    QObject::connect(&mStreamingWorker, SIGNAL(dropWarning(const char*, const char*)), this, SLOT(displayWarning(const char*, const char*)));
    QObject::connect(&mStreamingWorker, SIGNAL(dropInfo(const char*, const char*)), this, SLOT(displayInfo(const char*, const char*)));
}

MainWindow::~MainWindow()
{
    MainWindow::on_pushButton_close_webcam_clicked();
    delete ui;
}

void MainWindow::updateScreen(const cv::Mat& frame_mat)
{
    qt_image = QImage(frame_mat.data, frame_mat.cols, frame_mat.rows, QImage::Format_BGR888);
    ui->label->setPixmap(QPixmap::fromImage(qt_image));
    ui->label->resize(ui->label->pixmap().size());
}

void MainWindow::displayError(const char* title, const char* message)
{
    QMessageBox::critical(this, tr(title), tr(message));
}

void MainWindow::displayWarning(const char* title, const char* message)
{
    QMessageBox::warning(this, tr(title), tr(message));
}

void MainWindow::displayInfo(const char* title, const char* message)
{
    QMessageBox::information(this, tr(title), tr(message));
}

void MainWindow::on_pushButton_open_webcam_clicked()
{
    if (mStreamingWorker.getRunning())
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
        return;
    }
    try
    {
        MainWindow::connectToRtspServer();
    }
    catch (const std::runtime_error& e)
    {
        MainWindow::displayError("Connect to RTSP server failed", e.what());
        return;
    }
}

void MainWindow::on_pushButton_close_webcam_clicked()
{
    if (!mStreamingWorker.getRunning())
    {
        return;
    }
    cv::Mat image = cv::Mat::zeros(mStreamingWorker.getFrameSize(),CV_8UC3);
    MainWindow::updateScreen(image);
    try
    {
        MainWindow::rtspTeardown();
    }
    catch (const std::exception& e)
    {
        MainWindow::displayError("Teardown failed", e.what());
    }
    mStreamingWorker.stop();
    if (rtsp_client)
    {
        IClient* tmp = rtsp_client.release();
        delete tmp;
    }
    mStreamingWorker.stopRtpClient();
}

void MainWindow::parseRtspUrl() noexcept(false)
{
    const char* url = rtsp_url.c_str();
    if (std::strncmp(url, "rtsp://", 7) != 0)
    {
        throw std::runtime_error(
            "Bad format of RTSP URL.\n"\
            "URL must have rtsp :// at the beginning."
        );
    }
    // parse url
    uint16_t port = 0;
    char ip[RESPOND_WORD_MAX_LEN] = { 0 };
    char suffix[RESPOND_WORD_MAX_LEN] = { 0 };
    //
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

void MainWindow::on_lineEdit_textChanged(const QString& arg1)
{
    rtsp_url = arg1.toStdString();
}

void MainWindow::on_lineEdit_returnPressed()
{
    MainWindow::on_pushButton_open_webcam_clicked();
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
        rtsp_client = std::make_unique<CTcpClient>(rtsp_ip.c_str(), rtsp_port);
        rtsp_client->initClient();
    }
}

void MainWindow::connectToRtspServer() noexcept(false)
{
    mStreamingWorker.setRtpClientCast(true);
    MainWindow::rtspOptions();        //throws errors
    MainWindow::rtspDescribe();       //throws errors
    MainWindow::rtspSetup();          //throws errors
    mStreamingWorker.setRtpClientIp(rtp_mcast_ip.c_str());
    mStreamingWorker.setRtpClientPort(rtp_mcast_port);
    mStreamingWorker.initRtpClient(); //throws errors
    mStreamingWorker.start();         //throws errors
    MainWindow::rtspPlay();           //throws errors
}

void MainWindow::rtspOptions() noexcept(false)
{
    CTcpClient& clientRtsp = *rtsp_client.get();
    std::string sendingMessage;
    {
        std::stringstream ss;
        ss << MethodToString[static_cast<size_t>(RtspMethod::OPTIONS)] << ' ' << rtsp_url << ' ' << RTSP_VERSION << RECURSION_NEWLINE;
        ss << "CSeq: " << ++rtsp_cseq_ctr << RECURSION_NEWLINE;
        ss << "User - Agent: RtspClient / v1.0.0 (by Jovan Slavujevic)" << DOUBLE_RECURSION_NEWLINE;
        sendingMessage = ss.str();
    }
    clientRtsp << sendingMessage;
    uint16_t ret = (uint16_t)(clientRtsp >> &rtsp_message);
    rtsp_message.setCurrentSize(ret);
    const std::vector<const char*>& responseLine = ::getResponseLines(rtsp_message.cData());
    if (!::parseRtspResponmseFirstTwoLines(rtsp_message.cData(), responseLine[0], rtsp_cseq_ctr) )
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
    IClient& clientRtsp = *rtsp_client.get();
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
    uint16_t ret = (uint16_t)(clientRtsp >> &rtsp_message);
    rtsp_message.setCurrentSize(ret);
    const std::vector<const char*>& responseLine = ::getResponseLines(rtsp_message.cData());
    if (!::parseRtspResponmseFirstTwoLines(rtsp_message.cData(), responseLine[0], rtsp_cseq_ctr))
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
    if (std::strlen(SDP) != (size_t)contentLength)
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
    rtp_mcast_port = ret;
    rtsp_format = response_word[1]; // RTP/AVP
    const char* networkLine = std::strstr(videoLine, "c=IN");
    if (!networkLine)
    {
        throw std::runtime_error("Bad RTSP DESCRIBE response -> Missing network line.");
    }
    scanf_ret = sscanf_s(networkLine, "%s %s %[^/]/%hu\r\n", response_word[0], RESPOND_WORD_MAX_LEN, response_word[1], RESPOND_WORD_MAX_LEN, response_word[3], RESPOND_WORD_MAX_LEN, &ret);
    if (scanf_ret != 4 || strcmp(response_word[1], "IP4"))
    {
        throw std::runtime_error("Bad RTSP DESCRIBE response -> Bad format of network line.");
    }
    rtp_mcast_ip = response_word[2];
}

void MainWindow::rtspSetup() noexcept(false)
{
    IClient& clientRtsp = *rtsp_client.get();
    std::string sendingMessage;
    {
        std::stringstream ss;
        ss << MethodToString[static_cast<size_t>(RtspMethod::SETUP)] << ' ' << rtsp_url << ' ' << RTSP_VERSION << RECURSION_NEWLINE;
        ss << "CSeq: " << ++rtsp_cseq_ctr << RECURSION_NEWLINE;
        ss << "User - Agent: RtspClient / v1.0.0 (by Jovan Slavujevic)" << RECURSION_NEWLINE;
        ss << "Transport: " << rtsp_format << ";multicast;port=" << rtp_mcast_port << "-0" << DOUBLE_RECURSION_NEWLINE;
        sendingMessage = ss.str();
    }
    clientRtsp << sendingMessage;
    uint16_t rcvBytes;
    uint16_t ret[2];
    rcvBytes = (uint16_t)(clientRtsp >> &rtsp_message);
    rtsp_message.setCurrentSize(rcvBytes);
    const std::vector<const char*>& responseLine = ::getResponseLines(rtsp_message.cData());
    if (!::parseRtspResponmseFirstTwoLines(rtsp_message.cData(), responseLine[0], rtsp_cseq_ctr))
    {
        throw std::runtime_error("Bad format of 1st/2nd line of RTSP SETUP response.");
    }
    int scanf_ret;
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
    if (scanf_ret != 7 || strcmp(response_word[0], "Transport:") || (rtsp_format != response_word[1]) || strcmp(response_word[2], "multicast"))
    {
        std::stringstream ss;
        ss << "Bad format of 3rd line of RTSP SETUP response.\n\n" << std::string(responseLine[1], std::strstr(responseLine[1], RECURSION_NEWLINE));
        throw std::runtime_error(ss.str());
    }
    scanf_ret = sscanf_s(response_word[3], "%[^=]=%s", response_word[0], RESPOND_WORD_MAX_LEN, response_word[1], RESPOND_WORD_MAX_LEN);
    if (scanf_ret != 2 || std::strcmp(response_word[0], "destination"))
    {
        throw std::runtime_error("Bad format of destination line of RTSP SETUP response.");
    }
    rtp_mcast_ip = response_word[1];
    scanf_ret = sscanf_s(response_word[5], "%[^=]=%hu-%hu", response_word[0], RESPOND_WORD_MAX_LEN, ret, ret + 1);
    if (scanf_ret != 3 || strcmp(response_word[0], "port") || ret[1] != 0)
    {
        throw std::runtime_error("Bad format of port line of RTSP SETUP response.");
    }
    rtp_mcast_port = ret[0];
    scanf_ret = sscanf_s(responseLine[2], "%s %hu", response_word[0], RESPOND_WORD_MAX_LEN, ret);
    if (scanf_ret != 2)
    {
        throw std::runtime_error("Bad format of 4th line of RTSP SETUP response.");
    }
    rtsp_session = ret[0];
}

void MainWindow::rtspPlay() noexcept(false)
{
    IClient& clientRtsp = *rtsp_client.get();
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
    uint16_t ret = (uint16_t)(clientRtsp >> &rtsp_message);
    rtsp_message.setCurrentSize(ret);
    const std::vector<const char*>& responseLine = ::getResponseLines(rtsp_message.cData());
    if (!::parseRtspResponmseFirstTwoLines(rtsp_message.cData(), responseLine[0], rtsp_cseq_ctr))
    {
        throw std::runtime_error("Bad format of 1st/2nd line of RTSP DESCRIBE response.");
    }
}

void MainWindow::rtspTeardown() noexcept(false)
{
    IClient& clientRtsp = *rtsp_client.get();
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
    uint16_t ret = (uint16_t)(clientRtsp >> &rtsp_message);
    rtsp_message.setCurrentSize(ret);
    const std::vector<const char*>& responseLine = ::getResponseLines(rtsp_message.cData());
    if (!::parseRtspResponmseFirstTwoLines(rtsp_message.cData(), responseLine[0], rtsp_cseq_ctr))
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

static bool parseRtspResponmseFirstTwoLines(const char* line1, const char* line2, uint16_t cseqCtr)
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