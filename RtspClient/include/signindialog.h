#ifndef SIGNINDIALOG_H
#define SIGNINDIALOG_H

#include <string>
#include <memory>

#include <QDialog>

namespace Ui {
class SignInDialog;
}

class NetworkUserWorker;
class MainWindow;

class SignInDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SignInDialog(NetworkUserWorker* network_worker, QWidget* parent = nullptr);
    ~SignInDialog();

private slots:
    void on_username_email_line_edit_textEdited(const QString &arg1);
    void on_password_line_edit_textEdited(const QString &arg1);
    void on_username_email_line_edit_returnPressed();
    void on_password_line_edit_returnPressed();
    void on_sign_in_push_button_clicked();
    void on_cancel_push_button_clicked();
    void on_clicked_label();

signals:
    void dropError(std::string title, std::string message);
    void dropWarning(std::string title, std::string message);
    void dropInfo(std::string title, std::string message);

private:
    Ui::SignInDialog* ui;
    NetworkUserWorker* worker;
    MainWindow* parent;
    std::string username;
    std::string password;
};

#endif // SIGNINDIALOG_H
