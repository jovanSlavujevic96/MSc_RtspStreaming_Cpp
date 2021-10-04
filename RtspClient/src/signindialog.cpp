#include "network_user_worker.h"
#include "signindialog.h"
#include "ui_signindialog.h"

SignInDialog::SignInDialog(NetworkUserWorker* network_worker, QWidget *parent) :
    QDialog(parent),
    worker{network_worker},
    ui(new Ui::SignInDialog)
{
    ui->setupUi(this);
    ui->username_email_label->setToolTip("Enter your username or e-mail address");
    ui->username_email_line_edit->setToolTip("Enter your username or e-mail address");
    ui->password_line_edit->setToolTip("Enter your password");
    ui->password_label->setToolTip("Enter your password");
    ui->sign_up_label->setToolTip("You don't have account? Go to sign up.");
}

SignInDialog::~SignInDialog()
{
    delete ui;
}

void SignInDialog::on_sign_up_label_linkActivated(const QString &link)
{

}


void SignInDialog::on_username_email_line_edit_textEdited(const QString &arg1)
{
    username = arg1.toStdString();
}


void SignInDialog::on_password_line_edit_textEdited(const QString &arg1)
{
    password = arg1.toStdString();
}


void SignInDialog::on_username_email_line_edit_returnPressed()
{
    SignInDialog::on_sign_in_push_button_clicked();
}


void SignInDialog::on_password_line_edit_returnPressed()
{
    SignInDialog::on_sign_in_push_button_clicked();
}

void SignInDialog::on_sign_in_push_button_clicked()
{
    if (username == "" || password == "")
    {
        worker->dropError("Sign in", "Empty username or password fields.");
        return;
    }
    try
    {
        worker->signIn(username, password);
    }
    catch (const std::exception& e)
    {
        worker->dropError("Sign in failed", e.what());
        return;
    }
    worker->start();
    while(!worker->isRunning());
    try
    {
        worker->askForList();
    }
    catch (const std::exception& e)
    {
        worker->dropError("Sign in failed", e.what());
        return;
    }
    QDialog::close();
}


void SignInDialog::on_cancel_push_button_clicked()
{
    QDialog::close();
}
