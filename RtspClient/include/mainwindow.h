#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <memory>
#include <thread>

#include <QMainWindow>
#include <QTimer>

#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

#include "iclient.h"
#include "ctcp_client.h"

#include "rtsp_url_info.h"
#include "rtsp_parser.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_pushButton_open_webcam_clicked();

    void on_pushButton_close_webcam_clicked();

    void update_window();

    void on_lineEdit_textChanged(const QString &arg1);

    void on_pushButton_clicked();

    void on_lineEdit_returnPressed();

private:
    Ui::MainWindow *ui;

    QTimer *timer;
    cv::VideoCapture cap;

    cv::Mat frame;
    QImage qt_image;

    RtspUrlInfo rtsp_info;
    RtspParser rtsp_parser;
    std::unique_ptr<IClient> rtp_client;
    std::unique_ptr<CTcpClient> rtsp_client;
    std::thread rtsp_communication_thread;
    bool rtsp_loop_running;

    void connectRtspClient();
    void runRtspCommunication();
};

#endif // MAINWINDOW_H
