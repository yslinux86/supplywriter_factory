#include <QCryptographicHash>
#include <QDate>

#include "foremanauth.h"
#include "logindialog.h"
#include "maindialog.h"
#include "ui_foremanauth.h"

//领班信息输入对话框
ForeManAuth::ForeManAuth(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ForeManAuth)
{
    ui->setupUi(this);
    setWindowFlags(Qt::WindowMinimizeButtonHint |
                   Qt::WindowCloseButtonHint);

    this->init_dialog();
}

ForeManAuth::~ForeManAuth()
{
    delete val;
    delete ui;
}

void ForeManAuth::init_dialog()
{
    LoginDialog login;
    login.set_style_sheet(this, "light.qss");

    pixmap.load(":/images/logo.png");
    ui->logo->setScaledContents(false);
    ui->logo->setPixmap(pixmap);

    QFont font1("Microsoft YaHei", 13, 80);
    ui->title->setFont(font1);
    QFont font3("Microsoft YaHei", 16, 65);
    ui->formanauth->setFont(font3);

    ui->ForemanPasswd->setEchoMode(QLineEdit::Password);
    ui->ForemanName->setFocus();
    ui->OKButton->setDefault(true);

    val = new QRegExpValidator(QRegExp("[0-9]+$"));
    ui->PlannedNumber->setValidator(val);
    ui->FirstNumber->setValidator(val);

    for (quint8 kk = 0; kk < sizeof(info)/sizeof(struct supplyinfo); kk++)
    {
        ui->ComponentNo->addItem(info[kk].componentid);
        ui->ComponentNo->setItemData(kk, info[kk].model+"  "+info[kk].type+"  "+QString::number(info[kk].pages), Qt::ToolTipRole);
    }
}

void ForeManAuth::mousePressEvent(QMouseEvent *event)
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

void ForeManAuth::mouseMoveEvent(QMouseEvent *event)
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

void ForeManAuth::mouseReleaseEvent(QMouseEvent *event)
{
    //放下左键即停止移动
    if (event->button() == Qt::LeftButton)
    {
        is_drag = false;
    }
}

void ForeManAuth::on_OKButton_clicked()
{
    if (ui->ForemanName->text().length() == 0 ||
        ui->ForemanPasswd->text().length() == 0)
    {
        ui->ForemanAuthState->setText("<p style=\"color:red;font-weight:bold\">用户名或密码为空！</p>");
        return;
    }

    QString strmd5_username = QCryptographicHash::hash(ui->ForemanName->text().toLocal8Bit(), QCryptographicHash::Md5).toHex();
    QString strmd5_password = QCryptographicHash::hash(ui->ForemanPasswd->text().toLocal8Bit(), QCryptographicHash::Md5).toHex();

    setting.beginGroup(strmd5_username);
    if (setting.value("name").toString().compare(strmd5_username, Qt::CaseInsensitive) != 0)
    {
        ui->ForemanAuthState->setText("<p style=\"color:red;font-weight:bold\">该用户不存在！</p>");
        setting.endGroup();
        return;
    }
    if (setting.value("pwd").toString().compare(strmd5_password, Qt::CaseInsensitive) != 0)
    {
        ui->ForemanAuthState->setText("<p style=\"color:red;font-weight:bold\">密码不正确！</p>");
        setting.endGroup();
        return;
    }
    //更新最近使用日期
    QString strmd5_recently = QCryptographicHash::hash(QDate::currentDate().toString("yyyy/M/d").toLocal8Bit(), QCryptographicHash::Md5).toHex();
    setting.setValue("recent", strmd5_recently);
    setting.endGroup();

    if (ui->PlannedNumber->text().length() == 0 ||
        ui->FirstNumber->text().length() == 0)
    {
        ui->ForemanAuthState->setText("<p style=\"color:red;font-weight:bold\">计划生产总数或初始序号为空！</p>");
        return;
    }
    if (ui->PlannedNumber->text().toUInt() == 0)
    {
        ui->ForemanAuthState->setText("<p style=\"color:red;font-weight:bold\">计划生产总数应大于 0！</p>");
        return;
    }
    if (ui->PlannedNumber->text().toUInt() + ui->FirstNumber->text().toUInt() > 10000)
    {
        ui->ForemanAuthState->setText("<p style=\"color:red;font-weight:bold\">计划生产芯片序列号可能越界！</p>");
        return;
    }

    MainDialog *main = new MainDialog();
    connect(this, SIGNAL(send_work_content(QString, QString, QString, quint32, quint32)),
            main, SLOT(get_work_content(QString, QString, QString, quint32, quint32)));
    emit send_work_content(this->operator_name, ui->ForemanName->text(),
                           ui->ComponentNo->currentText(), ui->PlannedNumber->text().toUInt(), ui->FirstNumber->text().toUInt());
    main->show();

    this->accept();
}

void ForeManAuth::on_CancelButton_clicked()
{
    this->accept();
}

void ForeManAuth::get_operator_name(QString name)
{
    this->operator_name = name;
}
