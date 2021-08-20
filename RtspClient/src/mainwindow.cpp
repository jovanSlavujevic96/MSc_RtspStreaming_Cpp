#include <iostream>
#include <sstream>

#include "rtsp_package.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QLineEdit>
#include <QMessageBox>

#define APP_NAME "RTSP Client"
#define DEFAULT_RTSP_URL "rtsp://127.0.0.1:9090/live"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->lineEdit->setClearButtonEnabled(true);

    timer = new QTimer(this);

    rtsp_info.url = DEFAULT_RTSP_URL;
    rtsp_loop_running = false;
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButton_open_webcam_clicked()
{
    cap.open(0);

    if(!cap.isOpened())  // Check if we succeeded
    {
//        std::cout << "camera is not open" << std::endl;
    }
    else
    {
//        std::cout << "camera is open" << std::endl;

        connect(timer, SIGNAL(timeout()), this, SLOT(update_window()));
        timer->start(20);
    }
}

void MainWindow::on_pushButton_close_webcam_clicked()
{
    disconnect(timer, SIGNAL(timeout()), this, SLOT(update_window()));
    cap.release();

    cv::Mat image = cv::Mat::zeros(frame.size(),CV_8UC3);

    qt_image = QImage((const unsigned char*) (image.data), image.cols, image.rows, QImage::Format_RGB888);

    ui->label->setPixmap(QPixmap::fromImage(qt_image));

    ui->label->resize(ui->label->pixmap().size());

//    std::cout << "camera is closed" << std::endl;
}

void MainWindow::update_window()
{
    cap >> frame;

    cv::cvtColor(frame, frame, cv::COLOR_BGR2RGB);

    qt_image = QImage((const unsigned char*) (frame.data), frame.cols, frame.rows, QImage::Format_RGB888);

    ui->label->setPixmap(QPixmap::fromImage(qt_image));

    ui->label->resize(ui->label->pixmap().size());
}

void MainWindow::on_lineEdit_textChanged(const QString &arg1)
{
    rtsp_info.url = arg1.toStdString();
}

void MainWindow::on_pushButton_clicked()
{
    MainWindow::connectRtspClient();
}

void MainWindow::on_lineEdit_returnPressed()
{
    MainWindow::connectRtspClient();
}

void MainWindow::connectRtspClient()
{
    bool isParsed = rtsp_parser.parseRtspUrl(rtsp_info.url.c_str(), &rtsp_info);
    if (!isParsed)
    {
        /* throw an error */
        QMessageBox::critical(this, tr(APP_NAME), tr("Parsing of RTSP URL has been FAILED!"));
        return;
    }
    rtsp_parser.setRtspUrlInfo(&rtsp_info);

    if (rtsp_communication_thread.joinable())
    {
        rtsp_communication_thread.detach();
        if (rtsp_client)
        {
            rtsp_client.release();
        }
        rtsp_communication_thread.join();
    }
    rtsp_client = std::make_unique<CTcpClient>(rtsp_info.ip.c_str(), rtsp_info.port);
    try
    {
        rtsp_client->initClient();
    }
    catch (const CSocketException& e)
    {
        /* throw an error */
        QMessageBox::critical(this, tr(APP_NAME), tr(e.what()));
        return;
    }
    rtsp_loop_running = true;
    rtsp_communication_thread = std::thread(&MainWindow::runRtspCommunication, this);
}

void MainWindow::runRtspCommunication()
{
    CTcpClient& client_rtsp = *rtsp_client.get();

    std::string toSend;
    RtspPackage toReceive;

    rtsp_parser.setStartingMethod();

    while (rtsp_loop_running)
    {
        try
        {
            rtsp_parser.formRtspRequest(toSend);
            client_rtsp << toSend;
            client_rtsp >> &toReceive;
            rtsp_parser.parserRtspResponse(toReceive.cData());
        }
        catch (const std::runtime_error& e)
        {
            /* throw an warning, then reset communication */
            QMessageBox::warning(this, tr(APP_NAME), tr(e.what()));
            rtsp_parser.setStartingMethod();
            continue;
        }
        catch (const CSocketException& e)
        {
            /* throw an error */
            QMessageBox::critical(this, tr(APP_NAME), tr(e.what()));
            rtsp_loop_running = false;
            break;
        }
    }
    rtsp_client.release();
    rtsp_client = nullptr;
}
