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
    void updateScreen(const cv::Mat& frame_mat);
    void displayError(const char* title, const char* message);
    void displayWarning(const char* title, const char* message);
    void displayInfo(const char* title, const char* message);

private slots:
    void on_pushButton_open_webcam_clicked();
    void on_pushButton_close_webcam_clicked();
    void on_lineEdit_textChanged(const QString& arg1);
    void on_lineEdit_returnPressed();

private:
    Ui::MainWindow *ui;

    QImage qt_image;
    StreamingWorker mStreamingWorker;

    std::unique_ptr<CTcpClient> rtsp_client;
    std::string rtsp_url;
    std::string rtsp_ip;
    std::string rtsp_suffix;
    uint16_t rtsp_port;
    uint16_t rtsp_cseq_ctr;
    uint32_t rtsp_session;
    std::string rtsp_format;
    RtspPackage rtsp_message;

    std::string rtp_mcast_ip;
    uint16_t rtp_mcast_port;

    void parseRtspUrl() noexcept(false);
    void initRtspClient() noexcept(false);
    void connectToRtspServer() noexcept(false);

    void rtspOptions() noexcept(false);
    void rtspDescribe() noexcept(false);
    void rtspSetup() noexcept(false);
    void rtspPlay() noexcept(false);
    void rtspTeardown() noexcept(false);
};

#endif // MAINWINDOW_H
