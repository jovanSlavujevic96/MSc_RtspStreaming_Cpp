#include "mainwindow.h"
#include "network_user_worker.h"
#include "signindialog.h"
#include "ui_signindialog.h"
#include "clickable_label.h"

SignInDialog::SignInDialog(NetworkUserWorker* network_worker, QWidget* widget_parent) :
    QDialog(widget_parent),
    ui(new Ui::SignInDialog),
    worker{network_worker},
    parent{ dynamic_cast<MainWindow*>(widget_parent) }
{
    ui->setupUi(this);
    ui->username_email_label->setToolTip("Enter your username or e-mail address");
    ui->username_email_line_edit->setToolTip("Enter your username or e-mail address");
    ui->password_line_edit->setToolTip("Enter your password");
    ui->password_label->setToolTip("Enter your password");
    ui->sign_up_label->setToolTip("You don't have account? Go to sign up.");

    QObject::connect(ui->sign_up_label, SIGNAL(clicked()), this, SLOT(on_clicked_label()));

    QObject::connect(this, SIGNAL(dropError(std::string, std::string)), parent, SLOT(displayError(std::string, std::string)));
    QObject::connect(this, SIGNAL(dropWarning(std::string, std::string)), parent, SLOT(displayWarning(std::string, std::string)));
    QObject::connect(this, SIGNAL(dropInfo(std::string, std::string)), parent, SLOT(displayInfo(std::string, std::string)));

}

SignInDialog::~SignInDialog()
{
    delete ui;
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
        emit dropError("Sign in", "Empty credential fields.");
        return;
    }
    try
    {
        worker->signIn(username, password);
    }
    catch (const std::exception& e)
    {
        emit dropError("Sign in failed", e.what());
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
        emit dropError("Sign in failed", e.what());
        return;
    }
    QDialog::close();
}

void SignInDialog::on_cancel_push_button_clicked()
{
    QDialog::close();
}

void SignInDialog::on_clicked_label()
{
    QDialog::close();
    parent->displaySignUpWindow();
}
