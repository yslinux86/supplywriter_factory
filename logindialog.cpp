#include <QFile>
#include <QApplication>
#include <QSettings>
#include <QStyle>
#include <QStyleFactory>
#include <QTextCodec>
#include <QCryptographicHash>

#include "maindialog.h"
#include "logindialog.h"
#include "foremanauth.h"
#include "ui_logindialog.h"

//操作员输入对话框
LoginDialog::LoginDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LoginDialog)
{
    ui->setupUi(this);
    setWindowFlags(Qt::WindowMinimizeButtonHint |
                   Qt::WindowCloseButtonHint);

    ui->stackedWidget->setCurrentIndex(0);
    this->init_dialog();

    this->create_user_init();
    this->renew_passwd_init();
    this->reset_passwd_init();
}

void LoginDialog::create_user_init()
{
    ui->Password1->setEchoMode(QLineEdit::Password);
    ui->Password2->setEchoMode(QLineEdit::Password);

    QFont font3("Microsoft YaHei", 16, 65);
    ui->createuser->setFont(font3);
}

void LoginDialog::renew_passwd_init()
{
    ui->renew_oldpasswd->setEchoMode(QLineEdit::Password);
    ui->renew_newpasswd->setEchoMode(QLineEdit::Password);
    ui->renew_newpasswdagain->setEchoMode(QLineEdit::Password);

    QFont font3("Microsoft YaHei", 16, 65);
    ui->renew_passwd->setFont(font3);
}

void LoginDialog::reset_passwd_init()
{
    ui->reset_passwd1->setEchoMode(QLineEdit::Password);
    ui->reset_passwd2->setEchoMode(QLineEdit::Password);

    QFont font3("Microsoft YaHei", 16, 65);
    ui->setupnewpassswd1->setFont(font3);
    ui->setupnewpassswd2->setFont(font3);

    ui->CreateDate->setDate(QDate::currentDate());
    ui->LastDate->setDate(QDate::currentDate());
}

LoginDialog::~LoginDialog()
{
    delete ui;
}

void LoginDialog::set_style_sheet(QWidget* w, QString filename)
{
    QString qssfile = QString(":/qss/%1").arg(filename);
    QFile skinfile(qssfile);
    skinfile.open(QIODevice::ReadOnly);
    w->setStyleSheet(skinfile.readAll());
    skinfile.close();
}

void LoginDialog::init_dialog()
{
    this->set_style_sheet(this, "light.qss");

    pixmap[0].load(":/images/logo.png");
    ui->logo->setScaledContents(false);
    ui->logo->setPixmap(pixmap[0]);

    QFont font1("Microsoft YaHei", 13, 80);
    ui->title->setFont(font1);
    QFont font2("Microsoft YaHei", 15, 70);
    ui->welcome->setFont(font2);
    QFont font3("Microsoft YaHei", 16, 65);
    ui->loginlabel->setFont(font3);

    ui->Password->setEchoMode(QLineEdit::Password);
    ui->Login->setDefault(true);
    ui->UserName->setFocus();
}

void LoginDialog::on_Login_clicked()
{
    if (ui->UserName->text().length() == 0 ||
        ui->Password->text().length() == 0)
    {
        ui->LoginState->setText("<p style=\"color:red;font-weight:bold\">用户名或密码为空！</p>");
        return;
    }

    QString strmd5_username = QCryptographicHash::hash(ui->UserName->text().toLocal8Bit(), QCryptographicHash::Md5).toHex();
    QString strmd5_password = QCryptographicHash::hash(ui->Password->text().toLocal8Bit(), QCryptographicHash::Md5).toHex();

    setting.beginGroup(strmd5_username);
    if (setting.value("name").toString().compare(strmd5_username, Qt::CaseInsensitive) != 0)
    {
        ui->LoginState->setText("<p style=\"color:red;font-weight:bold\">该用户不存在！</p>");
        setting.endGroup();
        return;
    }
    if (setting.value("pwd").toString().compare(strmd5_password, Qt::CaseInsensitive) != 0)
    {
        ui->LoginState->setText("<p style=\"color:red;font-weight:bold\">密码不正确！</p>");
        setting.endGroup();
        return;
    }
    //更新最近使用日期
    QString strmd5_recently = QCryptographicHash::hash(QDate::currentDate().toString("yyyy/M/d").toLocal8Bit(), QCryptographicHash::Md5).toHex();
    setting.setValue("recent", strmd5_recently);
    login_user = ui->UserName->text();
    setting.endGroup();

    ForeManAuth* auth = new ForeManAuth();
    connect(this, SIGNAL(send_operator_name(QString)), auth, SLOT(get_operator_name(QString)));
    emit send_operator_name(ui->UserName->text());
    auth->show();

    this->accept();
}

void LoginDialog::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        is_drag = true;
        //获得鼠标的初始位置
        mouse_start_point = event->globalPos();
        //获得窗口的初始位置
        window_start_point = this->frameGeometry().topLeft();
    }
}

void LoginDialog::mouseMoveEvent(QMouseEvent *event)
{
    //判断是否在拖拽移动
    if (is_drag)
    {
        //获得鼠标移动的距离
        QPoint move_distance = event->globalPos() - mouse_start_point;
        //改变窗口的位置
        this->move(window_start_point + move_distance);
    }
}

void LoginDialog::mouseReleaseEvent(QMouseEvent *event)
{
    //放下左键即停止移动
    if (event->button() == Qt::LeftButton)
    {
        is_drag = false;
    }
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QApplication::setStyle(QStyleFactory::create("fusion"));

    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));

    QCoreApplication::setOrganizationName("SupplyWriter");
    QCoreApplication::setApplicationName("user");

    QSettings setting;
    setting.setValue("desc", "supply writer user info");

    qputenv("QT_SCALE_FACTOR", "1.5");

    LoginDialog w;
    w.show();

    return a.exec();
}

void LoginDialog::clear_login_page()
{
    ui->UserName->clear();
    ui->Password->clear();
    ui->Login->setDefault(true);
    ui->LoginState->clear();
    ui->UserName->setFocus();
}

void LoginDialog::clear_create_page()
{
    ui->NewUserName->clear();
    ui->Password1->clear();
    ui->Password2->clear();
    ui->CreateUserOKButton->setDefault(true);
    ui->create_info->clear();
    ui->NewUserName->setFocus();
}

void LoginDialog::clear_renew_page()
{
    ui->renew_username->clear();
    ui->renew_oldpasswd->clear();
    ui->renew_newpasswd->clear();
    ui->renew_newpasswdagain->clear();
    ui->RenewPasswdOKButton->setDefault(true);
    ui->renewpasswdstate->clear();
    ui->renew_username->setFocus();
}

void LoginDialog::clear_reset_page()
{
    ui->reset_username->clear();
    ui->UserAuthState->clear();
    ui->CreateDate->setDate(QDate::currentDate());
    ui->LastDate->setDate(QDate::currentDate());
    ui->reset_username->setFocus();
    ui->UserAuthOKButton->setDefault(true);
}

void LoginDialog::clear_resetpwd_page()
{
    ui->reset_passwd1->clear();
    ui->reset_passwd2->clear();
    ui->resetpasswdstate->clear();
    ui->reset_passwd1->setFocus();
    ui->ResetPasswdOKButton->setDefault(true);
}

void LoginDialog::on_CreateUser_clicked()
{
    ui->stackedWidget->setCurrentIndex(1);
    this->clear_create_page();
}

void LoginDialog::on_BackButton_clicked()
{
    ui->stackedWidget->setCurrentIndex(0);
    this->clear_login_page();
}

void LoginDialog::on_ResetPasswdBackButton_clicked()
{
    ui->stackedWidget->setCurrentIndex(0);
    this->clear_login_page();
}

void LoginDialog::on_RenewPasswd_clicked()
{
    ui->stackedWidget->setCurrentIndex(2);
    this->clear_renew_page();
}

void LoginDialog::on_ResetPasswd_clicked()
{
    ui->stackedWidget->setCurrentIndex(3);
    this->clear_reset_page();
}

void LoginDialog::on_UserAuthOKButton_clicked()
{
    if (ui->reset_username->text().length() == 0)
    {
        ui->UserAuthState->setText("<p style=\"color:red;font-weight:bold\">用户名为空！</p>");
        return;
    }

    resetpwd_username = QCryptographicHash::hash(ui->reset_username->text().toLocal8Bit(), QCryptographicHash::Md5).toHex();

//    qDebug() << resetpwd_username;
    setting.beginGroup(resetpwd_username);
    if (setting.value("name").toString().compare(resetpwd_username, Qt::CaseInsensitive) != 0)
    {
        ui->UserAuthState->setText("<p style=\"color:red;font-weight:bold\">该用户不存在！</p>");
        setting.endGroup();
        return;
    }

    QString strmd5_register = QCryptographicHash::hash(ui->CreateDate->text().toLocal8Bit(), QCryptographicHash::Md5).toHex();
    QString strmd5_recently = QCryptographicHash::hash(ui->LastDate->text().toLocal8Bit(), QCryptographicHash::Md5).toHex();

    if (setting.value("origin").toString().compare(strmd5_register, Qt::CaseInsensitive) != 0)
    {
        ui->UserAuthState->setText("<p style=\"color:red;font-weight:bold\">注册日期不正确，验证失败！</p>");
        setting.endGroup();
        return;
    }

    if (setting.value("recent").toString().compare(strmd5_recently, Qt::CaseInsensitive) != 0)
    {
        ui->UserAuthState->setText("<p style=\"color:red;font-weight:bold\">最近使用日期不正确，验证失败！</p>");
        setting.endGroup();
        return;
    }
    setting.endGroup();

    ui->stackedWidget->setCurrentIndex(4);
    this->clear_resetpwd_page();
}

void LoginDialog::on_CreateUserOKButton_clicked()
{
    if (ui->NewUserName->text().length() == 0 ||
        ui->Password1->text().length() == 0 ||
        ui->Password2->text().length() == 0)
    {
        ui->create_info->setText("<p style=\"color:red;font-weight:bold\">用户名或密码为空！</p>");
        return;
    }
    if (ui->Password1->text().compare(ui->Password2->text(), Qt::CaseSensitive) != 0)
    {
        ui->create_info->setText("<p style=\"color:red;font-weight:bold\">输入密码不一致！</p>");
        return;
    }
    QString strmd5_username = QCryptographicHash::hash(ui->NewUserName->text().toLocal8Bit(), QCryptographicHash::Md5).toHex();
    QString strmd5_password = QCryptographicHash::hash(ui->Password1->text().toLocal8Bit(), QCryptographicHash::Md5).toHex();
    QString strmd5_register = QCryptographicHash::hash(QDate::currentDate().toString("yyyy/M/d").toLocal8Bit(), QCryptographicHash::Md5).toHex();
    QString strmd5_recently = QCryptographicHash::hash(QDate::currentDate().toString("yyyy/M/d").toLocal8Bit(), QCryptographicHash::Md5).toHex();

//    qDebug() << strmd5_username;
    setting.beginGroup(strmd5_username);
    if (setting.value("name").toString().compare(strmd5_username, Qt::CaseInsensitive) == 0)
    {
        ui->create_info->setText("<p style=\"color:red;font-weight:bold\">该用户已存在，不能重复创建！</p>");
        setting.endGroup();
        return;
    }
    setting.setValue("name", strmd5_username);
    setting.setValue("pwd", strmd5_password);
    setting.setValue("origin", strmd5_register);
    setting.setValue("recent", strmd5_recently);
    setting.endGroup();

    ui->create_info->setText("<p style=\"color:green;font-weight:bold\">注册成功，将在 3 秒后返回登录页面</p>");
    ui->CreateUserOKButton->setDisabled(true);
    ui->BackButton->setDisabled(true);
    this->Sleep(3000);

    ui->stackedWidget->setCurrentIndex(0);
    ui->CreateUserOKButton->setEnabled(true);
    ui->BackButton->setEnabled(true);
    this->clear_login_page();
}

void LoginDialog::Sleep(int msec)
{
    QTime dieTime = QTime::currentTime().addMSecs(msec);

    while(QTime::currentTime() < dieTime)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
}

void LoginDialog::on_RenewPasswdOKButton_clicked()
{
    if (ui->renew_username->text().length() == 0 ||
        ui->renew_oldpasswd->text().length() == 0 ||
        ui->renew_newpasswd->text().length() == 0 ||
        ui->renew_newpasswdagain->text().length() == 0)
    {
        ui->renewpasswdstate->setText("<p style=\"color:red;font-weight:bold\">输入信息不能为空！</p>");
        return;
    }

    if (ui->renew_newpasswd->text().compare(ui->renew_newpasswdagain->text(), Qt::CaseSensitive) != 0)
    {
        ui->renewpasswdstate->setText("<p style=\"color:red;font-weight:bold\">密码不一致</p>");
        return;
    }

    QString strmd5_username = QCryptographicHash::hash(ui->renew_username->text().toLocal8Bit(), QCryptographicHash::Md5).toHex();
    QString strmd5_password = QCryptographicHash::hash(ui->renew_oldpasswd->text().toLocal8Bit(), QCryptographicHash::Md5).toHex();
    QString strmd5_newpwd = QCryptographicHash::hash(ui->renew_newpasswd->text().toLocal8Bit(), QCryptographicHash::Md5).toHex();

    setting.beginGroup(strmd5_username);
    if (setting.value("name").toString().compare(strmd5_username, Qt::CaseInsensitive) != 0)
    {
        ui->renewpasswdstate->setText("<p style=\"color:red;font-weight:bold\">该用户不存在！</p>");
        setting.endGroup();
        return;
    }

    if (setting.value("pwd").toString().compare(strmd5_password, Qt::CaseInsensitive) != 0)
    {
        ui->renewpasswdstate->setText("<p style=\"color:red;font-weight:bold\">原密码不正确！</p>");
        setting.endGroup();
        return;
    }

    setting.setValue("pwd", strmd5_newpwd);
    setting.endGroup();

    ui->renewpasswdstate->setText("<p style=\"color:green;font-weight:bold\">密码修改成功，将在 3 秒后返回登录页面</p>");
    ui->RenewPasswdOKButton->setDisabled(true);
    ui->RenewPasswdBackButton->setDisabled(true);
    this->Sleep(3000);

    ui->stackedWidget->setCurrentIndex(0);
    ui->RenewPasswdOKButton->setEnabled(true);
    ui->RenewPasswdBackButton->setEnabled(true);
    this->clear_login_page();
}

void LoginDialog::on_UserAuthBackButton_clicked()
{
    ui->stackedWidget->setCurrentIndex(0);
    this->clear_login_page();
}

void LoginDialog::on_RenewPasswdBackButton_clicked()
{
    ui->stackedWidget->setCurrentIndex(0);
    this->clear_login_page();
}

void LoginDialog::on_ResetPasswdOKButton_clicked()
{
    if (ui->reset_passwd1->text().length() == 0 ||
        ui->reset_passwd2->text().length() == 0)
    {
        ui->resetpasswdstate->setText("<p style=\"color:red;font-weight:bold\">密码不能为空！</p>");
        return;
    }

    if (ui->reset_passwd1->text().compare(ui->reset_passwd2->text(), Qt::CaseSensitive) != 0)
    {
        ui->resetpasswdstate->setText("<p style=\"color:red;font-weight:bold\">输入密码不一致！</p>");
        return;
    }

    QString strmd5_password = QCryptographicHash::hash(ui->reset_passwd1->text().toLocal8Bit(), QCryptographicHash::Md5).toHex();
    setting.beginGroup(resetpwd_username);
    setting.setValue("pwd", strmd5_password);
    setting.endGroup();

    ui->resetpasswdstate->setText("<p style=\"color:green;font-weight:bold\">密码重置成功，将在 3 秒后返回登录页面</p>");
    ui->ResetPasswdBackButton->setDisabled(true);
    ui->ResetPasswdOKButton->setDisabled(true);
    this->Sleep(3000);
    ui->ResetPasswdBackButton->setEnabled(true);
    ui->ResetPasswdOKButton->setEnabled(true);

    ui->stackedWidget->setCurrentIndex(0);
    this->clear_login_page();
}
