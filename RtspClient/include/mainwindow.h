#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <memory>
#include <string>

#include <QMainWindow>

#include <opencv2/opencv.hpp>

#include "rtsp_package.h"
#include "rtsp_method_enum.h"
#include "streaming_worker.h"

class CTcpClient;
class NetworkUser;

namespace Ui {
class MainWindow;
}

class MainWindow : 
    public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

public slots:
    void displayFrame(cv::Mat frame_mat);
    void displayError(std::string title, std::string message);
    void displayWarning(std::string title, std::string message);
    void displayInfo(std::string title, std::string message);

private slots:
    void on_pushButton_openStream_clicked();
    void on_pushButton_closeStream_clicked();
    void on_get_streams_button_clicked();
    void on_rtspStreams_listWidget_doubleClicked(const QModelIndex &index);
    void on_connectToManager_button_clicked();

private: //methods
    void startRtspStream();
    void stopRtspStream();

    bool isMulticast() const;

    void parseRtspUrl() noexcept(false);
    void initRtspClient() noexcept(false);
    void connectToRtspServer() noexcept(false);

    void rtspOptions() noexcept(false);
    void rtspDescribe() noexcept(false);
    void rtspSetup() noexcept(false);
    void rtspPlay() noexcept(false);
    void rtspTeardown() noexcept(false);

private: //fields
    Ui::MainWindow *ui;

    QImage qt_image;
    StreamingWorker mStreamingWorker;

    std::unique_ptr<CTcpClient> rtsp_client;
    std::unique_ptr<NetworkUser> network_user;
    std::string rtsp_url;
    std::string rtsp_ip;
    std::string rtsp_suffix;
    uint16_t rtsp_port;
    uint16_t rtsp_cseq_ctr;
    uint32_t rtsp_session;
    std::string rtsp_format;
    RtspPackage rtsp_message;

    std::string rtp_target_ip;
    uint16_t rtp_target_port;
    std::string rtp_running_ip;
    uint16_t rtp_running_port;
    bool is_multicast;
};

#endif // MAINWINDOW_H
