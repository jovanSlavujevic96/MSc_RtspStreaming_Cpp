#ifndef SIGNINDIALOG_H
#define SIGNINDIALOG_H

#include <string>
#include <memory>

#include <QDialog>

namespace Ui {
class SignInDialog;
}

class NetworkUserWorker;

class SignInDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SignInDialog(NetworkUserWorker* network_worker, QWidget *parent = nullptr);
    ~SignInDialog();

private slots:
    void on_sign_up_label_linkActivated(const QString &link);
    void on_username_email_line_edit_textEdited(const QString &arg1);
    void on_password_line_edit_textEdited(const QString &arg1);
    void on_username_email_line_edit_returnPressed();
    void on_password_line_edit_returnPressed();
    void on_sign_in_push_button_clicked();
    void on_cancel_push_button_clicked();

private:
    NetworkUserWorker* worker;
    std::string username;
    std::string password;
    Ui::SignInDialog *ui;
};

#endif // SIGNINDIALOG_H
