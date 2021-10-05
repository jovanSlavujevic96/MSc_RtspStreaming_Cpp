#include "mainwindow.h"
#include "network_user_worker.h"
#include "signupdialog.h"
#include "ui_signupdialog.h"
#include "clickable_label.h"

SignUpDialog::SignUpDialog(NetworkUserWorker* network_worker, QWidget* widget_parent) :
    QDialog(widget_parent),
    ui(new Ui::SignUpDialog),
    worker{ network_worker },
    parent{ dynamic_cast<MainWindow*>(widget_parent) }
{
    ui->setupUi(this);
    ui->username_label->setToolTip("Enter your username");
    ui->username_line_edit->setToolTip("Enter your username");
    ui->email_label->setToolTip("Enter your e-mail address");
    ui->email_line_edit->setToolTip("Enter your e-mail address");
    ui->password_line_edit->setToolTip("Enter your password");
    ui->password_label->setToolTip("Enter your password");
    ui->sign_in_label->setToolTip("You already have account? Go to sign in.");

    QObject::connect(ui->sign_in_label, SIGNAL(clicked()), this, SLOT(on_clicked_label()));

    QObject::connect(this, SIGNAL(dropError(std::string, std::string)), parent, SLOT(displayError(std::string, std::string)));
    QObject::connect(this, SIGNAL(dropWarning(std::string, std::string)), parent, SLOT(displayWarning(std::string, std::string)));
    QObject::connect(this, SIGNAL(dropInfo(std::string, std::string)), parent, SLOT(displayInfo(std::string, std::string)));
}

SignUpDialog::~SignUpDialog()
{
    delete ui;
}

void SignUpDialog::on_username_line_edit_textEdited(const QString &arg1)
{
    username = arg1.toStdString();
}

void SignUpDialog::on_email_line_edit_textEdited(const QString &arg1)
{
    email = arg1.toStdString();
}

void SignUpDialog::on_password_line_edit_textEdited(const QString &arg1)
{
    password = arg1.toStdString();
}


void SignUpDialog::on_username_line_edit_returnPressed()
{
    SignUpDialog::on_sign_up_push_button_clicked();
}

void SignUpDialog::on_email_line_edit_returnPressed()
{
    SignUpDialog::on_sign_up_push_button_clicked();
}

void SignUpDialog::on_password_line_edit_returnPressed()
{
    SignUpDialog::on_sign_up_push_button_clicked();
}

void SignUpDialog::on_sign_up_push_button_clicked()
{
    if (username == "" || email == "" || password == "")
    {
        emit dropError("Sign up", "Empty credential fields.");
        return;
    }
    try
    {
        worker->signUp(username, email, password);
    }
    catch (const std::exception& e)
    {
        emit dropError("Sign in failed", e.what());
        return;
    }
    emit dropInfo("Sing up", "Succesfully registered account.");
    QDialog::close();
    parent->displaySignInWindow();
}

void SignUpDialog::on_cancel_push_button_clicked()
{
    QDialog::close();
}

void SignUpDialog::on_clicked_label()
{
    QDialog::close();
    parent->displaySignInWindow();
}
