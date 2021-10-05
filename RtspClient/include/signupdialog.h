#ifndef SIGNUPDIALOG_H
#define SIGNUPDIALOG_H

#include <string>

#include <QDialog>

namespace Ui {
class SignUpDialog;
}

class NetworkUserWorker;
class MainWindow;

class SignUpDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SignUpDialog(NetworkUserWorker* network_worker, QWidget* widget_parent = nullptr);
    ~SignUpDialog();

private slots:
    void on_username_line_edit_textEdited(const QString &arg1);
    void on_email_line_edit_textEdited(const QString &arg1);
    void on_password_line_edit_textEdited(const QString &arg1);
    void on_username_line_edit_returnPressed();
    void on_email_line_edit_returnPressed();
    void on_password_line_edit_returnPressed();
    void on_sign_up_push_button_clicked();
    void on_cancel_push_button_clicked();
    void on_clicked_label();

signals:
    void dropError(std::string title, std::string message);
    void dropWarning(std::string title, std::string message);
    void dropInfo(std::string title, std::string message);

private:
    Ui::SignUpDialog* ui;
    NetworkUserWorker* worker;
    MainWindow* parent;
    std::string username;
    std::string email;
    std::string password;
};

#endif // SIGNUPDIALOG_H
