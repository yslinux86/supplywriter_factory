#include <QWidget>
#include <QDate>
#include <QDebug>

#include "common.h"
#include "logindialog.h"
#include "maindialog.h"
#include "foremanauth.h"
#include "readback.h"
#include "ui_maindialog.h"

static quint8 last_state[8];

MainDialog::MainDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::MainDialog)
{
    ui->setupUi(this);

    setWindowFlags(Qt::WindowMinimizeButtonHint |
                   Qt::WindowMaximizeButtonHint |
                   Qt::WindowCloseButtonHint);

    init_dialog();
    load_widgets();

    db = QSqlDatabase::addDatabase("QODBC", "main");
    player = new QMediaPlayer;

    if (worker == NULL)
    {
        worker = new StateMonitor();
        connect(worker, SIGNAL(DB_Connect_Signal(quint8)), this, SLOT(slotGetDBStatus(quint8)));
        connect(this, SIGNAL(send_db_config(QString, QString, QString, QString)),
                worker, SLOT(on_get_sql_config(QString, QString, QString, QString)));
    }

    if (!worker->isRunning())
        worker->start(QThread::NormalPriority);

    timer[0] = new QTimer(this);
    connect(timer[0], SIGNAL(timeout()), this, SLOT(update_connect_fixture()));
    timer[0]->start(3000);

    timer[1] = new QTimer(this);
    connect(timer[1], SIGNAL(timeout()), this, SLOT(update_ui_info()));
    timer[1]->start(300000);
}

void MainDialog::update_timestamp()
{
    QDate date = QDate::currentDate();
    year = date.year();
    month = date.month();
    day = date.day();

    QString datestring = QString("%1-%2-%3").arg(year).arg(month).arg(day);
    ui->date_label->setText(datestring);
}

void MainDialog::update_serialno()
{
    serial_id = current_number;

    serial_no[0] = QString("%1%2%3%4%5%6%7").arg(serial_prefix).arg(ComponentNo).arg(serial_trade)
            .arg(serial_year[year-2021]).arg(serial_month[month-1]).arg(serial_day[day-1]).arg(serial_id++, 4, 10, QLatin1Char('0'));
    serial_no[1] = QString("%1%2%3%4%5%6%7").arg(serial_prefix).arg(ComponentNo).arg(serial_trade)
            .arg(serial_year[year-2021]).arg(serial_month[month-1]).arg(serial_day[day-1]).arg(serial_id++, 4, 10, QLatin1Char('0'));
    serial_no[2] = QString("%1%2%3%4%5%6%7").arg(serial_prefix).arg(ComponentNo).arg(serial_trade)
            .arg(serial_year[year-2021]).arg(serial_month[month-1]).arg(serial_day[day-1]).arg(serial_id++, 4, 10, QLatin1Char('0'));
    serial_no[3] = QString("%1%2%3%4%5%6%7").arg(serial_prefix).arg(ComponentNo).arg(serial_trade)
            .arg(serial_year[year-2021]).arg(serial_month[month-1]).arg(serial_day[day-1]).arg(serial_id++, 4, 10, QLatin1Char('0'));

    ui->Chip1SerialNo->setText(serial_no[0]);
    ui->Chip2SerialNo->setText(serial_no[1]);
    ui->Chip3SerialNo->setText(serial_no[2]);
    ui->Chip4SerialNo->setText(serial_no[3]);

    if (ui->EightChips->isChecked())
    {
        serial_no[4] = QString("%1%2%3%4%5%6%7").arg(serial_prefix).arg(ComponentNo).arg(serial_trade)
                .arg(serial_year[year-2021]).arg(serial_month[month-1]).arg(serial_day[day-1]).arg(serial_id++, 4, 10, QLatin1Char('0'));
        serial_no[5] = QString("%1%2%3%4%5%6%7").arg(serial_prefix).arg(ComponentNo).arg(serial_trade)
                .arg(serial_year[year-2021]).arg(serial_month[month-1]).arg(serial_day[day-1]).arg(serial_id++, 4, 10, QLatin1Char('0'));
        serial_no[6] = QString("%1%2%3%4%5%6%7").arg(serial_prefix).arg(ComponentNo).arg(serial_trade)
                .arg(serial_year[year-2021]).arg(serial_month[month-1]).arg(serial_day[day-1]).arg(serial_id++, 4, 10, QLatin1Char('0'));
        serial_no[7] = QString("%1%2%3%4%5%6%7").arg(serial_prefix).arg(ComponentNo).arg(serial_trade)
                .arg(serial_year[year-2021]).arg(serial_month[month-1]).arg(serial_day[day-1]).arg(serial_id++, 4, 10, QLatin1Char('0'));

        lineedit[0]->setText(serial_no[4]);
        lineedit[1]->setText(serial_no[5]);
        lineedit[2]->setText(serial_no[6]);
        lineedit[3]->setText(serial_no[7]);
    }
}

void MainDialog::update_ui_info()
{
    this->update_timestamp();
    this->update_serialno();
}

void MainDialog::load_widgets()
{
    QFile file(QSL(":/5_8booth.ui"));
    if (!file.open(QIODevice::ReadOnly))
        return;
    m_Widget = uiload.load(&file, this);
    file.close();

    ui->verticalLayout_15->insertWidget(2, m_Widget);
    m_Widget->hide();

    checkbox[0] = m_Widget->findChild<QCheckBox*>(QSL("CheckChip5"));
    checkbox[1] = m_Widget->findChild<QCheckBox*>(QSL("CheckChip6"));
    checkbox[2] = m_Widget->findChild<QCheckBox*>(QSL("CheckChip7"));
    checkbox[3] = m_Widget->findChild<QCheckBox*>(QSL("CheckChip8"));

    connect(checkbox[0], SIGNAL(stateChanged(int)), this, SLOT(CheckChip5_stateChanged(int)));
    connect(checkbox[1], SIGNAL(stateChanged(int)), this, SLOT(CheckChip6_stateChanged(int)));
    connect(checkbox[2], SIGNAL(stateChanged(int)), this, SLOT(CheckChip7_stateChanged(int)));
    connect(checkbox[3], SIGNAL(stateChanged(int)), this, SLOT(CheckChip8_stateChanged(int)));

    readbutton[0] = m_Widget->findChild<QPushButton*>(QSL("ReadChip5"));
    readbutton[1] = m_Widget->findChild<QPushButton*>(QSL("ReadChip6"));
    readbutton[2] = m_Widget->findChild<QPushButton*>(QSL("ReadChip7"));
    readbutton[3] = m_Widget->findChild<QPushButton*>(QSL("ReadChip8"));

    connect(readbutton[0], SIGNAL(clicked()), this, SLOT(ReadChip5_clicked()));
    connect(readbutton[1], SIGNAL(clicked()), this, SLOT(ReadChip6_clicked()));
    connect(readbutton[2], SIGNAL(clicked()), this, SLOT(ReadChip7_clicked()));
    connect(readbutton[3], SIGNAL(clicked()), this, SLOT(ReadChip8_clicked()));

    writebutton[0] = m_Widget->findChild<QPushButton*>(QSL("WriteChip5"));
    writebutton[1] = m_Widget->findChild<QPushButton*>(QSL("WriteChip6"));
    writebutton[2] = m_Widget->findChild<QPushButton*>(QSL("WriteChip7"));
    writebutton[3] = m_Widget->findChild<QPushButton*>(QSL("WriteChip8"));

    connect(writebutton[0], SIGNAL(clicked()), this, SLOT(WriteChip5_clicked()));
    connect(writebutton[1], SIGNAL(clicked()), this, SLOT(WriteChip6_clicked()));
    connect(writebutton[2], SIGNAL(clicked()), this, SLOT(WriteChip7_clicked()));
    connect(writebutton[3], SIGNAL(clicked()), this, SLOT(WriteChip8_clicked()));

    lineedit[0] = m_Widget->findChild<QLineEdit*>(QSL("Chip5SerialNo"));
    lineedit[1] = m_Widget->findChild<QLineEdit*>(QSL("Chip6SerialNo"));
    lineedit[2] = m_Widget->findChild<QLineEdit*>(QSL("Chip7SerialNo"));
    lineedit[3] = m_Widget->findChild<QLineEdit*>(QSL("Chip8SerialNo"));

    lineedit[0]->setFocusPolicy(Qt::NoFocus);
    lineedit[1]->setFocusPolicy(Qt::NoFocus);
    lineedit[2]->setFocusPolicy(Qt::NoFocus);
    lineedit[3]->setFocusPolicy(Qt::NoFocus);

    label[0] = m_Widget->findChild<QLabel*>(QSL("booth5state"));
    label[1] = m_Widget->findChild<QLabel*>(QSL("booth6state"));
    label[2] = m_Widget->findChild<QLabel*>(QSL("booth7state"));
    label[3] = m_Widget->findChild<QLabel*>(QSL("booth8state"));
}

void MainDialog::update_connect_fixture()
{
    if (checkIpValid(checkIPversion(ui->FixtureIPAddr->text()), ui->FixtureIPAddr->text()))
    {
        server_status = check_server_status();
    }
    else
    {
        server_status = _INVALID_PARA;
    }

    Update_FixtureStatus();
}

void MainDialog::Update_FixtureStatus()
{
    if (server_status == _FAILED_STATUS)
    {
        qDebug() << __func__ << "offline";
        ui->fixture_state_label->setText("<p style=\"color:red;font-weight:bold\">设备离线！</p>");
        ui->booth1state->setText(tr("<font style='color:FF0000; font:bold;'>%1</font>").arg(QStringLiteral("---------")));
        ui->booth2state->setText(tr("<font style='color:FF0000; font:bold;'>%1</font>").arg(QStringLiteral("---------")));
        ui->booth3state->setText(tr("<font style='color:FF0000; font:bold;'>%1</font>").arg(QStringLiteral("---------")));
        ui->booth4state->setText(tr("<font style='color:FF0000; font:bold;'>%1</font>").arg(QStringLiteral("---------")));
        if (ui->EightChips->isChecked())
        {
            for (quint8 kk = 0; kk < 4; kk++)
                label[kk]->setText(tr("<font style='color:FF0000; font:bold;'>%1</font>").arg(QStringLiteral("---------")));
        }

        for(quint8 jj = 0; jj < 8; jj++)
            last_state[jj] = _OFF_LINE;
    }
}

void MainDialog::init_dialog()
{
    LoginDialog login;
    login.set_style_sheet(this, "light.qss");

    this->update_timestamp();

    on_ChooseAuto_clicked();

    for (qint16 kk = 0; kk < 2; kk++)
    {
        ui->TradeMark->addItem(_trademark[kk]);
        ui->Manufactor->addItem(_trademark[kk]);
    }

    ui->Chip1SerialNo->setFocusPolicy(Qt::NoFocus);
    ui->Chip2SerialNo->setFocusPolicy(Qt::NoFocus);
    ui->Chip3SerialNo->setFocusPolicy(Qt::NoFocus);
    ui->Chip4SerialNo->setFocusPolicy(Qt::NoFocus);
    ui->ChipType->setFocusPolicy(Qt::NoFocus);
    ui->MarketingArea->setFocusPolicy(Qt::NoFocus);
    ui->TotalPages->setFocusPolicy(Qt::NoFocus);
    ui->ChipModelID->setFocusPolicy(Qt::NoFocus);
    ui->FreePages->setFocusPolicy(Qt::NoFocus);
    ui->BeyondPages->setFocusPolicy(Qt::NoFocus);

    ui->DBPasswd->setEchoMode(QLineEdit::Password);

    this->setTabOrder(ui->DBIPAddr, ui->DBUser);
    this->setTabOrder(ui->DBUser, ui->DBPasswd);
    this->setTabOrder(ui->DBPasswd, ui->DBSource);
    this->setTabOrder(ui->DBSource, ui->FixtureIPAddr);

    ui->DBIPAddr->setFocus();

    group[0] = new QButtonGroup;
    group[0]->addButton(ui->ChooseAuto);
    group[0]->addButton(ui->ChooseManual);

    group[1] = new QButtonGroup;
    group[1]->addButton(ui->ChooseLight);
    group[1]->addButton(ui->ChooseDeep);

    group[2] = new QButtonGroup;
    group[2]->addButton(ui->FourChips);
    group[2]->addButton(ui->EightChips);
}

MainDialog::~MainDialog()
{
    if (worker)
    {
        worker->quit();
        worker->wait(0);
    }

    if (tcpSocket[0])
         delete tcpSocket[0];
    if (tcpSocket[1])
        delete tcpSocket[1];

    delete timer[0];
    delete timer[1];
    delete player;
    delete group[0];
    delete group[1];
    delete group[2];

    delete ui;
}

QString MainDialog::calculate_checkcode(QString str)
{
    //计算校验码
    QString code;

    code = str;

    return code;
}

void MainDialog::mousePressEvent(QMouseEvent *event)
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

void MainDialog::mouseMoveEvent(QMouseEvent *event)
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

void MainDialog::mouseReleaseEvent(QMouseEvent *event)
{
    //放下左键即停止移动
    if (event->button() == Qt::LeftButton)
    {
        is_drag = false;
    }
}

void MainDialog::get_work_content(QString operator_name, QString foreman,
                                  QString component_id, quint32 planned_number, quint32 first_number)
{
    ui->OperatorID->setText(operator_name);
    ui->ForemanName->setText(foreman);
    ui->ComponentNo->setText(component_id);
    ui->PlannedNumber->setText(QString::number(planned_number));
    ui->CurrentNumber->setText(QString::number(first_number));

    ComponentNo = component_id;
    this->current_number = first_number;
    this->max_num = first_number + planned_number;

    this->update_serialno();

    for (qulonglong kk = 0; kk < sizeof(info) / sizeof(struct supplyinfo); kk++)
    {
        if (component_id.compare(info[kk].componentid, Qt::CaseSensitive) == 0)
        {
            ui->ChipType->setText(info[kk].type);
            ui->MarketingArea->setText(info[kk].marketing_area);
            ui->TotalPages->setText(QString::number(info[kk].pages));
            ui->ChipModelID->setText(info[kk].model);
            ui->FreePages->setText(QString::number(info[kk].freepages));
            ui->BeyondPages->setText(QString::number(info[kk].beyondpages));
            break;
        }
    }
}

void MainDialog::on_ChooseAuto_clicked()
{
    ui->ReadChip1->hide();
    ui->ReadChip2->hide();
    ui->ReadChip3->hide();
    ui->ReadChip4->hide();

    ui->WriteChip1->hide();
    ui->WriteChip2->hide();
    ui->WriteChip3->hide();
    ui->WriteChip4->hide();

    if (ui->EightChips->isChecked())
    {
        for (quint8 i = 0; i<4; i++)
        {
            readbutton[i]->hide();
            writebutton[i]->hide();
        }
    }
}

void MainDialog::on_ChooseManual_clicked()
{
    ui->ReadChip1->show();
    ui->ReadChip2->show();
    ui->ReadChip3->show();
    ui->ReadChip4->show();

    ui->WriteChip1->show();
    ui->WriteChip2->show();
    ui->WriteChip3->show();
    ui->WriteChip4->show();

    if (ui->EightChips->isChecked())
    {
        for (quint8 i = 0; i<4; i++)
        {
            readbutton[i]->show();
            writebutton[i]->show();
        }
    }
}

//检查IP地址是否合法
bool MainDialog::checkIpValid(int version, QString ip)
{
    if (version == 4)
    {
        QRegExp rx2("^((25[0-5]|2[0-4]\\d|[01]?\\d\\d?)\\.){3}(25[0-5]|2[0-4]\\d|[01]?\\d\\d?)$");
        if(rx2.exactMatch(ip))
        {
            //ip地址合法
            return true;
        }
    }
    else if (version == 6)
    {
        QRegExp rx2("^([\\da-fA-F]{1,4}:){6}((25[0-5]|2[0-4]\\d|[01]?\\d\\d?)\\.){3}"
                    "(25[0-5]|2[0-4]\\d|[01]?\\d\\d?)$|^::([\\da-fA-F]{1,4}:){0,4}"
                    "((25[0-5]|2[0-4]\\d|[01]?\\d\\d?)\\.){3}"
                    "(25[0-5]|2[0-4]\\d|[01]?\\d\\d?)$|^([\\da-fA-F]{1,4}:):([\\da-fA-F]{1,4}:){0,3}"
                    "((25[0-5]|2[0-4]\\d|[01]?\\d\\d?)\\.){3}"
                    "(25[0-5]|2[0-4]\\d|[01]?\\d\\d?)$|^([\\da-fA-F]{1,4}:){2}:([\\da-fA-F]{1,4}:){0,2}"
                    "((25[0-5]|2[0-4]\\d|[01]?\\d\\d?)\\.){3}"
                    "(25[0-5]|2[0-4]\\d|[01]?\\d\\d?)$|^([\\da-fA-F]{1,4}:){3}:([\\da-fA-F]{1,4}:){0,1}"
                    "((25[0-5]|2[0-4]\\d|[01]?\\d\\d?)\\.){3}"
                    "(25[0-5]|2[0-4]\\d|[01]?\\d\\d?)$|^([\\da-fA-F]{1,4}:){4}:"
                    "((25[0-5]|2[0-4]\\d|[01]?\\d\\d?)\\.){3}"
                    "(25[0-5]|2[0-4]\\d|[01]?\\d\\d?)$|^([\\da-fA-F]{1,4}:){7}[\\da-fA-F]{1,4}$|^:((:[\\da-fA-F]{1,4}){1,6}|:)$|^[\\da-fA-F]{1,4}:"
                    "((:[\\da-fA-F]{1,4}){1,5}|:)$|^([\\da-fA-F]{1,4}:){2}"
                    "((:[\\da-fA-F]{1,4}){1,4}|:)$|^([\\da-fA-F]{1,4}:){3}"
                    "((:[\\da-fA-F]{1,4}){1,3}|:)$|^([\\da-fA-F]{1,4}:){4}"
                    "((:[\\da-fA-F]{1,4}){1,2}|:)$|^([\\da-fA-F]{1,4}:){5}:([\\da-fA-F]{1,4})?$|^([\\da-fA-F]{1,4}:){6}:$");
        if(rx2.exactMatch(ip))
        {
            //ip地址合法
            return true;
        }
    }

    return false;
}

//检测IP地址的版本
int MainDialog::checkIPversion(QString IP)
{
    if (IP.contains(":", Qt::CaseInsensitive))
        return 6;
    else if (IP.contains(".", Qt::CaseInsensitive))
        return 4;

    return 0;
}

void MainDialog::Sleep(int msec)
{
    QTime dieTime = QTime::currentTime().addMSecs(msec);

    while(QTime::currentTime() < dieTime)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
}

void MainDialog::slotGetDBStatus(quint8 _odbc_status)
{
    if (odbc_status == _odbc_status)
        return;
    else
        odbc_status = _odbc_status;

    if (odbc_status == _FAILED_STATUS)
    {
        ui->db_state_label->setText(tr("<font style='color:red; font:bold;'>%1</font>").arg(QStringLiteral("连接失败！")));
    }
    else if (odbc_status == _SUCCESS_STATUS)
    {
        ui->db_state_label->setText(tr("<font style='color:green; font:bold;'>%1</font>").arg(QStringLiteral("连接成功！")));
    }
    else if (odbc_status == _INVALID_PARA)
    {
        ui->db_state_label->setText(tr("<font style='color:red; font:bold;'>%1</font>").arg(QStringLiteral("信息不正确！")));
    }
}

void MainDialog::slotConnected()
{
    this->server_status = _SUCCESS_STATUS;
}

void MainDialog::statusReceived()
{
    BoothState state;
    qint64 length;

    while (tcpSocket[0]->bytesAvailable() > 0)
    {
        QByteArray datagram;
        length = tcpSocket[0]->bytesAvailable();

        datagram.resize(tcpSocket[0]->bytesAvailable());
        tcpSocket[0]->read(datagram.data(), datagram.size());
        memcpy(&state, datagram.data(), length);
        tcpSocket[0]->close();

        if (last_state[0] != state.state[0])
        {
            if (state.state[0] == _NO_CHIP)
                ui->booth1state->setText(tr("<font style='color:#DC143C; font:bold;'>%1</font>").arg(QStringLiteral("未放置耗材")));
            else if (state.state[0] == _BLANK_CHIP)
                ui->booth1state->setText(tr("<font style='color:#556B2F; font:bold;'>%1</font>").arg(QStringLiteral("耗材就绪")));
            else
                ui->booth1state->setText(tr("<font style='color:#1E90FF; font:bold;'>%1</font>").arg(QStringLiteral("耗材已有数据")));

            last_state[0] = state.state[0];
        }

        if (last_state[1] != state.state[1])
        {
            if (state.state[1] == _NO_CHIP)
                ui->booth2state->setText(tr("<font style='color:#DC143C; font:bold;'>%1</font>").arg(QStringLiteral("未放置耗材")));
            else if (state.state[1] == _BLANK_CHIP)
                ui->booth2state->setText(tr("<font style='color:#556B2F; font:bold;'>%1</font>").arg(QStringLiteral("耗材就绪")));
            else
                ui->booth2state->setText(tr("<font style='color:#1E90FF; font:bold;'>%1</font>").arg(QStringLiteral("耗材已有数据")));

            last_state[1] = state.state[1];
        }

        if (last_state[2] != state.state[2])
        {
            if (state.state[2] == _NO_CHIP)
                ui->booth3state->setText(tr("<font style='color:#DC143C; font:bold;'>%1</font>").arg(QStringLiteral("未放置耗材")));
            else if (state.state[2] == _BLANK_CHIP)
                ui->booth3state->setText(tr("<font style='color:#556B2F; font:bold;'>%1</font>").arg(QStringLiteral("耗材就绪")));
            else
                ui->booth3state->setText(tr("<font style='color:#1E90FF; font:bold;'>%1</font>").arg(QStringLiteral("耗材已有数据")));

            last_state[2] = state.state[2];
        }

        if (last_state[3] != state.state[3])
        {
            if (state.state[3] == _NO_CHIP)
                ui->booth4state->setText(tr("<font style='color:#DC143C; font:bold;'>%1</font>").arg(QStringLiteral("未放置耗材")));
            else if (state.state[3] == _BLANK_CHIP)
                ui->booth4state->setText(tr("<font style='color:#556B2F; font:bold;'>%1</font>").arg(QStringLiteral("耗材就绪")));
            else
                ui->booth4state->setText(tr("<font style='color:#1E90FF; font:bold;'>%1</font>").arg(QStringLiteral("耗材已有数据")));

            last_state[3] = state.state[3];
        }

        if (ui->EightChips->isChecked() && state.mode == 8)
        {
            for (quint8 kk = 4; kk < 8; kk++)
            {
                if (last_state[kk] != state.state[kk])
                {
                    if (state.state[kk] == _NO_CHIP)
                        label[kk-4]->setText(tr("<font style='color:#DC143C; font:bold;'>%1</font>").arg(QStringLiteral("未放置耗材")));
                    else if (state.state[kk] == _BLANK_CHIP)
                        label[kk-4]->setText(tr("<font style='color:#556B2F; font:bold;'>%1</font>").arg(QStringLiteral("耗材就绪")));
                    else
                        label[kk-4]->setText(tr("<font style='color:#1E90FF; font:bold;'>%1</font>").arg(QStringLiteral("耗材已有数据")));

                    last_state[kk] = state.state[kk];
                }
            }
        }

        ui->fixture_state_label->setText(tr("<font style='color:green; font:bold;'>%1</font>").arg(QStringLiteral("治具连接正常！")));

        break;
    }
}

//定时发送检测治具状态信息
//返回false表示离线，返回true表示在线
bool MainDialog::check_server_status()
{
    if (tcpSocket[0] == NULL)
        tcpSocket[0] = new QTcpSocket(this);
    this->server_status = _FAILED_STATUS;
    MsgHdr hdr;

    connect(tcpSocket[0], SIGNAL(connected()), this, SLOT(slotConnected()));
    connect(tcpSocket[0], SIGNAL(readyRead()), this, SLOT(statusReceived()));
    tcpSocket[0]->connectToHost(ui->FixtureIPAddr->text(), TCP_PORT);

    hdr.cmd = OP_GET_BOOTH_STATE;   //获取治具上各卡座状态
    if (ui->FourChips->isChecked())
        hdr.len = 4;
    else if (ui->EightChips->isChecked())
        hdr.len = 8;

    if (ui->ChipModelID->text().mid(0, 3).compare("TL-", Qt::CaseSensitive) == 0)
        hdr.i2c_addr = _TONER_CHIP_ADDR;
    else if (ui->ChipModelID->text().mid(0, 3).compare("DL-", Qt::CaseSensitive) == 0)
        hdr.i2c_addr = _DRUM_CHIP_ADDR;

    if (tcpSocket[0]->write((const char*)&hdr, sizeof(hdr)) == -1)
    {
//        qDebug() << "write cmd failed";
        return _FAILED_STATUS;
    }

    this->Sleep(100);
    return server_status;
}

void MainDialog::on_FixtureIPAddr_textChanged(const QString &arg1)
{
    Q_UNUSED(arg1);

    if (ui->FixtureIPAddr->text().length() && checkIpValid(checkIPversion(ui->FixtureIPAddr->text()), ui->FixtureIPAddr->text()))
    {
        if (tcpSocket[0])
        {
            if (tcpSocket[0]->isOpen())
                tcpSocket[0]->close();
            delete tcpSocket[0];
            tcpSocket[0] = NULL;
        }

        if (check_server_status() == _FAILED_STATUS)
        {
//            qDebug() << "server state offline";
            ui->fixture_state_label->setText("<p style=\"color:red;font-weight:bold\">设备离线！</p>");
            ui->booth1state->setText(tr("<font style='color:FF0000; font:bold;'>%1</font>").arg(QStringLiteral("---------")));
            ui->booth2state->setText(tr("<font style='color:FF0000; font:bold;'>%1</font>").arg(QStringLiteral("---------")));
            ui->booth3state->setText(tr("<font style='color:FF0000; font:bold;'>%1</font>").arg(QStringLiteral("---------")));
            ui->booth4state->setText(tr("<font style='color:FF0000; font:bold;'>%1</font>").arg(QStringLiteral("---------")));
            if (ui->EightChips->isChecked())
            {
                for (quint8 kk = 0; kk < 4; kk++)
                    label[kk]->setText(tr("<font style='color:FF0000; font:bold;'>%1</font>").arg(QStringLiteral("---------")));
            }
            for(quint8 jj = 0; jj < 8; jj++)
                last_state[jj] = _OFF_LINE;
        }
    }
    else if (ui->FixtureIPAddr->text().length())
    {
//        qDebug() << "invalid ip addr";
        ui->fixture_state_label->setText("<p style=\"color:red;font-weight:bold\">IP地址不正确！</p>");
        ui->booth1state->setText(tr("<font style='color:FF0000; font:bold;'>%1</font>").arg(QStringLiteral("---------")));
        ui->booth2state->setText(tr("<font style='color:FF0000; font:bold;'>%1</font>").arg(QStringLiteral("---------")));
        ui->booth3state->setText(tr("<font style='color:FF0000; font:bold;'>%1</font>").arg(QStringLiteral("---------")));
        ui->booth4state->setText(tr("<font style='color:FF0000; font:bold;'>%1</font>").arg(QStringLiteral("---------")));
        if (ui->EightChips->isChecked())
        {
            for (quint8 kk = 0; kk < 4; kk++)
                label[kk]->setText(tr("<font style='color:FF0000; font:bold;'>%1</font>").arg(QStringLiteral("---------")));
        }
        for(quint8 jj = 0; jj < 8; jj++)
            last_state[jj] = _OFF_LINE;
    }
    else
    {
//        qDebug() << "blank";
        ui->fixture_state_label->clear();
        ui->booth1state->setText(tr("<font style='color:FF0000; font:bold;'>%1</font>").arg(QStringLiteral("---------")));
        ui->booth2state->setText(tr("<font style='color:FF0000; font:bold;'>%1</font>").arg(QStringLiteral("---------")));
        ui->booth3state->setText(tr("<font style='color:FF0000; font:bold;'>%1</font>").arg(QStringLiteral("---------")));
        ui->booth4state->setText(tr("<font style='color:FF0000; font:bold;'>%1</font>").arg(QStringLiteral("---------")));
        if (ui->EightChips->isChecked())
        {
            for (quint8 kk = 0; kk < 4; kk++)
                label[kk]->setText(tr("<font style='color:FF0000; font:bold;'>%1</font>").arg(QStringLiteral("---------")));
        }
        for(quint8 jj = 0; jj < 8; jj++)
            last_state[jj] = _OFF_LINE;
    }
}

void MainDialog::dataReceived()
{
    unsigned char resp[1024];
    qint64 length;

    while (tcpSocket[1]->bytesAvailable() > 0)
    {
        QByteArray datagram;
        length = tcpSocket[1]->bytesAvailable();

        datagram.resize(tcpSocket[1]->bytesAvailable());
        tcpSocket[1]->read(datagram.data(), datagram.size());
        memcpy(&resp, datagram.data(), length);
        tcpSocket[1]->close();

        if (((RespInfo*)resp)->ret == RESP_OK)
        {
            play_mp3_sound(QCoreApplication::applicationDirPath() + "/sound/done.mp3");
            if (((RespInfo*)resp)->cmd == OP_WRITE_INFO)
            {
                ui->fixture_state_label->setText("<p style=\"color:green;font-weight:bold\">写入成功！</p>");
            }
            else if (((RespInfo*)resp)->cmd == OP_READ_BOOTH_TONER ||
                     ((RespInfo*)resp)->cmd == OP_READ_BOOTH_DRUM)
            {
                ReadBoothInfo info;
                memcpy(&info, resp, sizeof(ReadBoothInfo));

                if (((RespInfo*)resp)->cmd == OP_READ_BOOTH_TONER)
                    ui->fixture_state_label->setText("<p style=\"color:green;font-weight:bold\">粉盒 读取成功！</p>");
                else if (((RespInfo*)resp)->cmd == OP_READ_BOOTH_DRUM)
                    ui->fixture_state_label->setText("<p style=\"color:green;font-weight:bold\">鼓组件 读取成功！</p>");

                ReadBack *readback = new ReadBack();

                connect(this, SIGNAL(sendChipInfo(struct cgprintech_supply_info_readback*)),
                        readback, SLOT(show_ChipInfo(struct cgprintech_supply_info_readback*)));
                connect(this, SIGNAL(sendChipBoothNo(quint8)), readback, SLOT(show_booth_index(quint8)));

                emit sendChipInfo(&info.info);
                emit sendChipBoothNo(info.index);

                readback->show();
            }
            this->Sleep(3000);
        }
        else
        {
            // operation failed
            if (((RespInfo*)resp)->cmd == OP_WRITE_INFO)
            {
                ui->fixture_state_label->setText("<p style=\"color:red;font-weight:bold\">写入失败！</p>");
            }
            else if (((RespInfo*)resp)->cmd == OP_READ_BOOTH_TONER ||
                     ((RespInfo*)resp)->cmd == OP_READ_BOOTH_DRUM)
            {
                if (((RespInfo*)resp)->cmd == OP_READ_BOOTH_TONER)
                {
                    ui->fixture_state_label->setText("<p style=\"color:red;font-weight:bold\">粉盒 读取失败！</p>");
                }
                else if (((RespInfo*)resp)->cmd == OP_READ_BOOTH_DRUM)
                {
                    ui->fixture_state_label->setText("<p style=\"color:red;font-weight:bold\">鼓组件 读取失败！</p>");
                }
            }
            play_mp3_sound(QCoreApplication::applicationDirPath() + "/sound/failed.mp3");

            this->Sleep(3000);
        }

        server_status = _INVALID_PARA;
        break;
    }
}

void MainDialog::play_mp3_sound(QString file)
{
    player->setMedia(QMediaContent(QUrl::fromLocalFile(file)));
    player->play();
}

void MainDialog::open_sql_server()
{
    if (db.isOpen())
    {
        //如果数据库句柄已打开，则先关闭
        db.close();
    }

    db.setHostName(ui->DBIPAddr->text());
    db.setUserName(ui->DBUser->text());
    db.setPassword(ui->DBPasswd->text());
    db.setDatabaseName(ui->DBSource->text());   //这里填写数据源名称

    if (db.open()) {
        ui->db_state_label->setText("<p style=\"color:green;font-weight:bold\">连接成功！</p>");
        odbc_status = _SUCCESS_STATUS;
        return;
    } else {
        ui->db_state_label->setText("<p style=\"color:red;font-weight:bold\">连接失败！</p>");
//        qDebug()<<"error open database because"<<db.lastError().text();
        odbc_status = _FAILED_STATUS;
        return;
    }
}

void MainDialog::try_connect_db()
{
    emit send_db_config(ui->DBIPAddr->text(), ui->DBUser->text(),
                        ui->DBPasswd->text(), ui->DBSource->text());

    if (ui->DBIPAddr->text().length() &&
        ui->DBUser->text().length() &&
        ui->DBPasswd->text().length() &&
        ui->DBSource->text().length() &&
        checkIpValid(checkIPversion(ui->DBIPAddr->text()), ui->DBIPAddr->text()))
    {
        open_sql_server();
    }
    else
    {
        odbc_status = _INVALID_PARA;
        ui->db_state_label->setText("<p style=\"color:red;font-weight:bold\">信息不正确！</p>");
    }
}

void MainDialog::on_DBIPAddr_textChanged(const QString &arg1)
{
    Q_UNUSED(arg1);
    Sleep(1500);
    try_connect_db();
}

void MainDialog::on_DBSource_textChanged(const QString &arg1)
{
    Q_UNUSED(arg1);
    Sleep(1500);
    try_connect_db();
}

void MainDialog::on_DBUser_textChanged(const QString &arg1)
{
    Q_UNUSED(arg1);
    Sleep(1500);
    try_connect_db();
}

void MainDialog::on_DBPasswd_textChanged(const QString &arg1)
{
    Q_UNUSED(arg1);
    Sleep(1500);
    try_connect_db();
}

void MainDialog::on_CheckChip1_stateChanged(int arg1)
{
    if (arg1 == 0)
        ui->Chip1SerialNo->setEnabled(false);
    else
        ui->Chip1SerialNo->setEnabled(true);
}

void MainDialog::on_CheckChip2_stateChanged(int arg1)
{
    if (arg1 == 0)
        ui->Chip2SerialNo->setEnabled(false);
    else
        ui->Chip2SerialNo->setEnabled(true);
}

void MainDialog::on_CheckChip3_stateChanged(int arg1)
{
    if (arg1 == 0)
        ui->Chip3SerialNo->setEnabled(false);
    else
        ui->Chip3SerialNo->setEnabled(true);
}

void MainDialog::on_CheckChip4_stateChanged(int arg1)
{
    if (arg1 == 0)
        ui->Chip4SerialNo->setEnabled(false);
    else
        ui->Chip4SerialNo->setEnabled(true);
}

void MainDialog::CheckChip5_stateChanged(int arg1)
{
    if (arg1 == 0)
        lineedit[0]->setEnabled(false);
    else
        lineedit[0]->setEnabled(true);
}

void MainDialog::CheckChip6_stateChanged(int arg1)
{
    if (arg1 == 0)
        lineedit[1]->setEnabled(false);
    else
        lineedit[1]->setEnabled(true);
}

void MainDialog::CheckChip7_stateChanged(int arg1)
{
    if (arg1 == 0)
        lineedit[2]->setEnabled(false);
    else
        lineedit[2]->setEnabled(true);
}

void MainDialog::CheckChip8_stateChanged(int arg1)
{
    if (arg1 == 0)
        lineedit[3]->setEnabled(false);
    else
        lineedit[3]->setEnabled(true);
}

void MainDialog::set_style_sheet(QString filename)
{
    QString qssfile = QString(":/qss/%1").arg(filename);
    QFile skinfile(qssfile);
    skinfile.open(QIODevice::ReadOnly);
    this->setStyleSheet(skinfile.readAll());
    skinfile.close();
}

void MainDialog::on_ChooseLight_clicked()
{
    this->set_style_sheet("light.qss");
}

void MainDialog::on_ChooseDeep_clicked()
{
    this->set_style_sheet("dark.qss");
}

void MainDialog::on_FourChips_clicked()
{
    if (chipmode == false)
        return;
    chipmode = false;

    serial_id -= 4;

    lineedit[0]->clear();
    lineedit[1]->clear();
    lineedit[2]->clear();
    lineedit[3]->clear();

    m_Widget->hide();
}

void MainDialog::on_EightChips_clicked()
{
    if (chipmode == true)
        return;
    chipmode = true;

    m_Widget->show();

    if (ui->ChooseAuto->isChecked())
    {
        for (qint8 i = 0; i < 4; i++)
        {
            readbutton[i]->hide();
            writebutton[i]->hide();
        }
    }
    else if (ui->ChooseManual->isChecked())
    {
        for (qint8 i = 0; i < 4; i++)
        {
            readbutton[i]->show();
            writebutton[i]->show();
        }
    }

    serial_no[4] = QString("%1%2%3%4%5%6%7").arg(serial_prefix).arg(ComponentNo).arg(serial_trade)
            .arg(serial_year[year-2021]).arg(serial_month[month-1]).arg(serial_day[day-1]).arg(serial_id++, 4, 10, QLatin1Char('0'));
    serial_no[5] = QString("%1%2%3%4%5%6%7").arg(serial_prefix).arg(ComponentNo).arg(serial_trade)
            .arg(serial_year[year-2021]).arg(serial_month[month-1]).arg(serial_day[day-1]).arg(serial_id++, 4, 10, QLatin1Char('0'));
    serial_no[6] = QString("%1%2%3%4%5%6%7").arg(serial_prefix).arg(ComponentNo).arg(serial_trade)
            .arg(serial_year[year-2021]).arg(serial_month[month-1]).arg(serial_day[day-1]).arg(serial_id++, 4, 10, QLatin1Char('0'));
    serial_no[7] = QString("%1%2%3%4%5%6%7").arg(serial_prefix).arg(ComponentNo).arg(serial_trade)
            .arg(serial_year[year-2021]).arg(serial_month[month-1]).arg(serial_day[day-1]).arg(serial_id++, 4, 10, QLatin1Char('0'));

    lineedit[0]->setText(serial_no[4]);
    lineedit[1]->setText(serial_no[5]);
    lineedit[2]->setText(serial_no[6]);
    lineedit[3]->setText(serial_no[7]);
}

//发送读写芯片数据
bool MainDialog::sendData(int cmd, void* data, int data_len)
{
    uint8_t writeinfo[sizeof(MsgHdr) + data_len];

    if (tcpSocket[1] == NULL)
        tcpSocket[1] = new QTcpSocket(this);

    connect(tcpSocket[1], SIGNAL(connected()), this, SLOT(slotConnected()));
    connect(tcpSocket[1], SIGNAL(readyRead()), this, SLOT(dataReceived()));

    if (tcpSocket[1]->isOpen() == true)
        tcpSocket[1]->close();

    tcpSocket[1]->connectToHost(ui->FixtureIPAddr->text(), TCP_PORT);

    ((MsgHdr*)writeinfo)->cmd = cmd;
    ((MsgHdr*)writeinfo)->len = data_len;
    if (data)
    {
        if (strncasecmp(((struct cgprintech_supply_info*)data)->model_id, "TL", 2) == 0)
            ((MsgHdr*)writeinfo)->i2c_addr = _TONER_CHIP_ADDR;
        else if (strncasecmp(((struct cgprintech_supply_info*)data)->model_id, "DL", 2) == 0)
            ((MsgHdr*)writeinfo)->i2c_addr = _DRUM_CHIP_ADDR;

        memcpy(writeinfo + sizeof(MsgHdr), data, data_len);
//        this->hex_dump((unsigned char*)&writeinfo, sizeof(writeinfo));
    }
    else
    {
        if (cmd == OP_READ_BOOTH_TONER)
            ((MsgHdr*)writeinfo)->i2c_addr = _TONER_CHIP_ADDR;
        else if (cmd == OP_READ_BOOTH_DRUM)
            ((MsgHdr*)writeinfo)->i2c_addr = _DRUM_CHIP_ADDR;
    }

    if (tcpSocket[1]->write((const char*)&writeinfo, sizeof(writeinfo)) == -1)
    {
//        qDebug() << "write to tcp socket failed";
        return false;
    }

    return true;
}

void MainDialog::on_ReadChip1_clicked()
{
    check_server_status();
    if (this->server_status == _FAILED_STATUS)
        return;

    //1.发送读取耗材信息命令
    if (ui->ComponentNo->text().mid(0, 2).compare("TL", Qt::CaseSensitive) == 0)
        this->sendData(OP_READ_BOOTH_TONER, NULL, 0);
    else
        this->sendData(OP_READ_BOOTH_DRUM, NULL, 0);

    return;
}

void MainDialog::on_ReadChip2_clicked()
{
    check_server_status();
    if (this->server_status == _FAILED_STATUS)
        return;

    //1.发送读取耗材信息命令
    if (ui->ComponentNo->text().mid(0, 2).compare("TL", Qt::CaseSensitive) == 0)
        this->sendData(OP_READ_BOOTH_TONER, NULL, 1);
    else
        this->sendData(OP_READ_BOOTH_DRUM, NULL, 1);

    return;
}

void MainDialog::on_ReadChip3_clicked()
{
    check_server_status();
    if (this->server_status == _FAILED_STATUS)
        return;

    //1.发送读取耗材信息命令
    if (ui->ComponentNo->text().mid(0, 2).compare("TL", Qt::CaseSensitive) == 0)
        this->sendData(OP_READ_BOOTH_TONER, NULL, 2);
    else
        this->sendData(OP_READ_BOOTH_DRUM, NULL, 2);

    return;
}

void MainDialog::on_ReadChip4_clicked()
{
    check_server_status();
    if (this->server_status == _FAILED_STATUS)
        return;

    //1.发送读取耗材信息命令
    if (ui->ComponentNo->text().mid(0, 2).compare("TL", Qt::CaseSensitive) == 0)
        this->sendData(OP_READ_BOOTH_TONER, NULL, 3);
    else
        this->sendData(OP_READ_BOOTH_DRUM, NULL, 3);

    return;
}

void MainDialog::ReadChip5_clicked()
{
    check_server_status();
    if (this->server_status == _FAILED_STATUS)
        return;

    //1.发送读取耗材信息命令
    if (ui->ComponentNo->text().mid(0, 2).compare("TL", Qt::CaseSensitive) == 0)
        this->sendData(OP_READ_BOOTH_TONER, NULL, 4);
    else
        this->sendData(OP_READ_BOOTH_DRUM, NULL, 4);

    return;
}

void MainDialog::ReadChip6_clicked()
{
    check_server_status();
    if (this->server_status == _FAILED_STATUS)
        return;

    //1.发送读取耗材信息命令
    if (ui->ComponentNo->text().mid(0, 2).compare("TL", Qt::CaseSensitive) == 0)
        this->sendData(OP_READ_BOOTH_TONER, NULL, 5);
    else
        this->sendData(OP_READ_BOOTH_DRUM, NULL, 5);

    return;
}

void MainDialog::ReadChip7_clicked()
{
    check_server_status();
    if (this->server_status == _FAILED_STATUS)
        return;

    //1.发送读取耗材信息命令
    if (ui->ComponentNo->text().mid(0, 2).compare("TL", Qt::CaseSensitive) == 0)
        this->sendData(OP_READ_BOOTH_TONER, NULL, 6);
    else
        this->sendData(OP_READ_BOOTH_DRUM, NULL, 6);

    return;
}

void MainDialog::ReadChip8_clicked()
{
    check_server_status();
    if (this->server_status == _FAILED_STATUS)
        return;

    //1.发送读取耗材信息命令
    if (ui->ComponentNo->text().mid(0, 2).compare("TL", Qt::CaseSensitive) == 0)
        this->sendData(OP_READ_BOOTH_TONER, NULL, 7);
    else
        this->sendData(OP_READ_BOOTH_DRUM, NULL, 7);

    return;
}

void MainDialog::on_WriteChip1_clicked()
{
    qDebug() << __func__;
}

void MainDialog::on_WriteChip2_clicked()
{
qDebug() << __func__;
}

void MainDialog::on_WriteChip3_clicked()
{
qDebug() << __func__;
}

void MainDialog::on_WriteChip4_clicked()
{
qDebug() << __func__;
}

void MainDialog::WriteChip5_clicked()
{
qDebug() << __func__;
}

void MainDialog::WriteChip6_clicked()
{
qDebug() << __func__;
}

void MainDialog::WriteChip7_clicked()
{
qDebug() << __func__;
}

void MainDialog::WriteChip8_clicked()
{
qDebug() << __func__;
}

void MainDialog::on_QueryInfo_clicked()
{
    //根据序列号查询数据库中耗材信息
}

void MainDialog::on_DeleteInfo_clicked()
{
    //根据序列号删除数据库中耗材记录
}
