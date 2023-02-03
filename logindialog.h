#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include <QSettings>

namespace Ui {
class LoginDialog;
}

class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoginDialog(QWidget *parent = nullptr);
    ~LoginDialog();
    void set_style_sheet(QWidget* w, QString filename);

protected:
    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);

signals:
    void send_operator_name(QString name);

private:
    Ui::LoginDialog *ui;
    bool is_drag = false;
    QPoint mouse_start_point;
    QPoint window_start_point;
    QPixmap pixmap[5];
    QSettings setting;
    QString login_user;
    QString resetpwd_username;

private:
    void init_dialog();
    void Sleep(int msec);
    void create_user_init();
    void renew_passwd_init();
    void reset_passwd_init();
    void clear_create_page();
    void clear_login_page();
    void clear_renew_page();
    void clear_reset_page();
    void clear_resetpwd_page();

private slots:
    void on_Login_clicked();
    void on_CreateUser_clicked();
    void on_BackButton_clicked();
    void on_ResetPasswdBackButton_clicked();
    void on_RenewPasswd_clicked();
    void on_ResetPasswd_clicked();
    void on_UserAuthOKButton_clicked();
    void on_CreateUserOKButton_clicked();
    void on_RenewPasswdOKButton_clicked();
    void on_UserAuthBackButton_clicked();
    void on_RenewPasswdBackButton_clicked();
    void on_ResetPasswdOKButton_clicked();
};

#endif // LOGINDIALOG_H
