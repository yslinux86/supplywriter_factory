#include <QWidget>
#include <QDate>
#include <QDebug>
#include <QSqlError>
#include <QMessageBox>
#include <QStandardItem>
#include <QStandardItemModel>

#include "common.h"
#include "logindialog.h"
#include "maindialog.h"
#include "foremanauth.h"
#include "sqlchipinfo.h"
#include "ui_maindialog.h"

#define QSL QStringLiteral

MainDialog::MainDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::MainDialog)
{
    ui->setupUi(this);
    setWindowFlags(Qt::WindowMinimizeButtonHint |
                   Qt::WindowMaximizeButtonHint |
                   Qt::WindowCloseButtonHint);
    init_dialog();

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

    scan_the_fixtures();

    timer[0] = new QTimer(this);
    timer[1] = new QTimer(this);
    timer[2] = new QTimer(this);
    timer[3] = new QTimer(this);
    timer[4] = new QTimer(this);

    connect(timer[0], SIGNAL(timeout()), this, SLOT(update_connect_fixture()));
    connect(timer[1], SIGNAL(timeout()), this, SLOT(update_ui_info()));
    connect(timer[2], SIGNAL(timeout()), this, SLOT(send_heartbeat_signal()));
    connect(timer[3], SIGNAL(timeout()), this, SLOT(scan_the_fixtures()));
    connect(timer[4], SIGNAL(timeout()), this, SLOT(check_fixture_online()));

    timer[4]->start(5000);
    timer[3]->start(6000);
    timer[2]->start(15000);
    timer[0]->start(3000);
    timer[1]->start(600000);
}

void MainDialog::udp_data_recv()
{
    BcInfoResp resp;
    qint64 length;

    while(udpSocket[0]->hasPendingDatagrams())
    {
        QByteArray datagram;
        length = udpSocket[0]->pendingDatagramSize();

        datagram.resize(udpSocket[0]->pendingDatagramSize());
        udpSocket[0]->readDatagram(datagram.data(), datagram.size());
        memcpy(&resp, datagram.data(), length);

        if (resp.resp.cmd == OP_BROADCAST_UDP_RESP)
        {
            bool flag = false;
            QString ipaddr = QString("%1").arg(resp.ipaddr);

            if (ui->RecommendFixtures->count() == 0)
            {
                ui->RecommendFixtures->addItem(ipaddr);
            }
            else
            {
                for (quint8 jj = 0; jj < ui->RecommendFixtures->count(); jj++)
                {
                    if (ui->RecommendFixtures->item(jj)->text().compare(ipaddr, Qt::CaseSensitive) == 0)
                    {
                        flag = true;
                        break;
                    }
                }

                if (flag == false)
                    ui->RecommendFixtures->addItem(ipaddr);
            }
        }
    }
}

void MainDialog::scan_the_fixtures()
{
    if (is_done)
    {
        timer[3]->stop();
        return;
    }

    //发送嗅探治具的广播报文，治具收到后回复自身IP地址
    if (udpSocket[0] == NULL)
    {
        udpSocket[0] = new QUdpSocket(this);
        connect(udpSocket[0], SIGNAL(readyRead()), this, SLOT(udp_data_recv()));
        bool ret = udpSocket[0]->bind(BC_UDP_PORT);
        if (!ret)
            return;
    }

    MsgHdr hdr;
    hdr.cmd = OP_BROADCAST_UDP_REQUEST;
    hdr.len = 0;
    hdr.i2c_addr = 0;

    udpSocket[0]->writeDatagram((char*)&hdr, sizeof(MsgHdr), QHostAddress::Broadcast, BC_UDP_PORT);
}

void MainDialog::udp_hb_recv()
{
    RespInfo resp;
    qint64 length;

    while(udpSocket[1]->hasPendingDatagrams())
    {
        QByteArray datagram;
        length = udpSocket[1]->pendingDatagramSize();

        datagram.resize(udpSocket[1]->pendingDatagramSize());
        udpSocket[1]->readDatagram(datagram.data(), datagram.size());
        memcpy(&resp, datagram.data(), length);

        if (resp.cmd == OP_HB_UDP_REQUEST)
            udp_hb_status = true;
    }
}

void MainDialog::send_udp_hb_pack(QString ipaddr)
{
    if (udpSocket[1] == NULL)
    {
        udpSocket[1] = new QUdpSocket(this);
        connect(udpSocket[1], SIGNAL(readyRead()), this, SLOT(udp_hb_recv()));
        bool ret = udpSocket[1]->bind(HB_UDP_PORT);
        if (!ret)
            return;
    }

    MsgHdr hdr;
    hdr.cmd = OP_HB_UDP_REQUEST;
    hdr.len = 0;
    hdr.i2c_addr = 0;

    udpSocket[1]->writeDatagram((char*)&hdr, sizeof(MsgHdr), QHostAddress(ipaddr), HB_UDP_PORT);
}

void MainDialog::check_fixture_online()
{
//    qDebug() << __func__ << ui->RecommendFixtures->count();
    if (ui->RecommendFixtures->count() == 0)
        return;

    int jj = ui->RecommendFixtures->count() - 1;
    do {
        udp_hb_status = false;
        send_udp_hb_pack(ui->RecommendFixtures->item(jj)->text());
        Sleep(500);
        if (udp_hb_status)
        {
//            qDebug() << jj << ui->RecommendFixtures->item(jj)->text() << "is online";
        }
        else
        {
//            qDebug() << jj << ui->RecommendFixtures->item(jj)->text() << "is offline";
            ui->RecommendFixtures->takeItem(jj);
        }

        if (jj == 0)
            break;

        jj--;
    } while(1);
}

void MainDialog::send_heartbeat_signal()
{
    if (is_done)
    {
        timer[2]->stop();
        return;
    }

    open_longconn_socket();
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

void MainDialog::clear_serialno_info()
{
    ui->Chip1SerialNo->clear();
    ui->Chip2SerialNo->clear();
    ui->Chip3SerialNo->clear();
    ui->Chip4SerialNo->clear();
}

//显示 num 个序列号
void MainDialog::update_serialno(quint8 num)
{
    serial_id = current_number;

    this->clear_serialno_info();

    serial_no[0] = QString("%1%2%3%4%5%6%7").arg(serial_prefix).arg(ComponentNo).arg(serial_trade)
            .arg(serial_year[year-2021]).arg(serial_month[month-1]).arg(serial_day[day-1]).arg(serial_id++, 4, 10, QLatin1Char('0'));
    serial_no[1] = QString("%1%2%3%4%5%6%7").arg(serial_prefix).arg(ComponentNo).arg(serial_trade)
            .arg(serial_year[year-2021]).arg(serial_month[month-1]).arg(serial_day[day-1]).arg(serial_id++, 4, 10, QLatin1Char('0'));
    serial_no[2] = QString("%1%2%3%4%5%6%7").arg(serial_prefix).arg(ComponentNo).arg(serial_trade)
            .arg(serial_year[year-2021]).arg(serial_month[month-1]).arg(serial_day[day-1]).arg(serial_id++, 4, 10, QLatin1Char('0'));
    serial_no[3] = QString("%1%2%3%4%5%6%7").arg(serial_prefix).arg(ComponentNo).arg(serial_trade)
            .arg(serial_year[year-2021]).arg(serial_month[month-1]).arg(serial_day[day-1]).arg(serial_id++, 4, 10, QLatin1Char('0'));

    ui->Chip1SerialNo->setText(serial_no[0]);
    if (num == 1)
        return;
    ui->Chip2SerialNo->setText(serial_no[1]);
    if (num == 2)
        return;
    ui->Chip3SerialNo->setText(serial_no[2]);
    if (num == 3)
        return;
    ui->Chip4SerialNo->setText(serial_no[3]);
    if (num == 4)
        return;
}

void MainDialog::update_ui_info()
{
    //软件打开隔日情况下，自动更新日期
    this->update_timestamp();

    if (!is_done)
    {
        int remain = ui->QuantityRemain->text().toInt();
        if (remain > 4)
            update_serialno(4);
        else
            update_serialno(remain);
    }
}

void MainDialog::update_connect_fixture()
{
    if (checkIpValid(checkIPversion(ui->FixtureIPAddr->text()), ui->FixtureIPAddr->text()))
    {
        server_status[0] = check_server_status();
        if (server_status[0] == _SUCCESS_STATUS && !is_done)
        {
//            qDebug() << "send bulk data to fixture";
            unsigned int len;
            BoothSupplyInfoW info;
            memset(&info, 0, sizeof(BoothSupplyInfoW));

            info.booth_num = 4;
            memcpy(&info.serial_no[0], ui->Chip1SerialNo->text().toLatin1().data(), ui->Chip1SerialNo->text().length());
            memcpy(&info.serial_no[1], ui->Chip2SerialNo->text().toLatin1().data(), ui->Chip2SerialNo->text().length());
            memcpy(&info.serial_no[2], ui->Chip3SerialNo->text().toLatin1().data(), ui->Chip3SerialNo->text().length());
            memcpy(&info.serial_no[3], ui->Chip4SerialNo->text().toLatin1().data(), ui->Chip4SerialNo->text().length());

            memcpy(&info.model_id, ui->ChipModelID->text().toLatin1().data(), ui->ChipModelID->text().length());
            memcpy(&info.marketing_area, ui->MarketingArea->text().toLatin1().data(), ui->MarketingArea->text().length());
            memcpy(&info.manufacturer, ui->manufacturer->currentText().toLatin1().data(), ui->manufacturer->currentText().length());
            memcpy(&info.trade_mark, ui->TradeMark->currentText().toLatin1().data(), ui->TradeMark->currentText().length());
            memcpy(&info.type, ui->ChipType->text().toLatin1().data(), ui->ChipType->text().length());

            QString current_date = QString("%1%2%3")
                    .arg(year, 4, 10, QLatin1Char('0'))
                    .arg(month, 2, 10, QLatin1Char('0'))
                    .arg(day, 2, 10, QLatin1Char('0'));
            StringToHex(current_date.toLatin1().data(), info.product_date, &len);
            Pack32(info.pages, ui->TotalPages->text().toUInt());
            Pack16(info.beyond_pages, ui->BeyondPages->text().toUInt());
            Pack16(info.free_pages, ui->FreePages->text().toUInt());

            //测试发现治具连接正常，发送批量芯片数据到治具
            sendData(OP_SEND_BULK_INFO, &info, sizeof(BoothSupplyInfoW));
        }
    }
    else
    {
        server_status[0] = _INVALID_PARA;
    }

    Update_FixtureStatus();
}

void MainDialog::Update_FixtureStatus()
{
    if (server_status[0] == _FAILED_STATUS)
    {
//        qDebug() << __func__ << "offline";
        ui->fixture_state_label->setText("<p style=\"color:red;font-weight:bold\">治具离线</p>");
    }
    else if (server_status[0] == _SUCCESS_STATUS)
    {
//        qDebug() << "server is ONLINE";
        ui->fixture_state_label->setText(tr("<font style='color:#01847e; font:bold;'>%1</font>").arg(QStringLiteral("治具在线")));
    }
}

void MainDialog::init_dialog()
{
    LoginDialog login;
    login.set_style_sheet(this, "light.qss");

    this->update_timestamp();

    for (qint16 kk = 0; kk < 2; kk++)
    {
        ui->TradeMark->addItem(_trademark[kk]);
        ui->manufacturer->addItem(_trademark[kk]);
    }

    ui->Chip1SerialNo->setReadOnly(true);
    ui->Chip2SerialNo->setReadOnly(true);
    ui->Chip3SerialNo->setReadOnly(true);
    ui->Chip4SerialNo->setReadOnly(true);

    ui->ChipType->setReadOnly(true);
    ui->MarketingArea->setReadOnly(true);
    ui->TotalPages->setReadOnly(true);
    ui->ChipModelID->setReadOnly(true);
    ui->FreePages->setReadOnly(true);
    ui->BeyondPages->setReadOnly(true);

    ui->DBPasswd->setEchoMode(QLineEdit::Password);

    this->setTabOrder(ui->DBIPAddr, ui->DBUser);
    this->setTabOrder(ui->DBUser, ui->DBPasswd);
    this->setTabOrder(ui->DBPasswd, ui->DBSource);
    this->setTabOrder(ui->DBSource, ui->FixtureIPAddr);

    ui->DBIPAddr->setFocus();

    ui->QueryInfo->setEnabled(false);
    ui->DeleteInfo->setEnabled(false);
}

MainDialog::~MainDialog()
{
    if (worker)
    {
        worker->quit();
        worker->wait(0);
    }

    int kk;
    for (kk = 0; kk < 3; kk++)
    {
        if (tcpSocket[kk])
             delete tcpSocket[kk];
    }

    for (kk = 0; kk < 2; kk++)
        if (udpSocket[kk])
            delete udpSocket[kk];

    for (kk = 0; kk < 5; kk++)
    {
        if (timer[kk]->isActive())
            timer[kk]->stop();
        delete timer[kk];
    }

    delete player;
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

    if (planned_number > 4)
        update_serialno(4);
    else
        update_serialno(planned_number);
    ui->QuantityRemain->setNum((int)planned_number);

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
                    "(25[0-5]|2[0-4]\\d|[01]?\\d\\d?)$|^([\\da-fA-F]{1,4}:){7}[\\da-fA-F]{1,4}$|^:"
                    "((:[\\da-fA-F]{1,4}){1,6}|:)$|^[\\da-fA-F]{1,4}:"
                    "((:[\\da-fA-F]{1,4}){1,5}|:)$|^([\\da-fA-F]{1,4}:){2}"
                    "((:[\\da-fA-F]{1,4}){1,4}|:)$|^([\\da-fA-F]{1,4}:){3}"
                    "((:[\\da-fA-F]{1,4}){1,3}|:)$|^([\\da-fA-F]{1,4}:){4}"
                    "((:[\\da-fA-F]{1,4}){1,2}|:)$|^([\\da-fA-F]{1,4}:){5}:"
                    "([\\da-fA-F]{1,4})?$|^([\\da-fA-F]{1,4}:){6}:$");
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
        ui->db_state_label->setText(tr("<font style='color:red; font:bold;'>%1</font>").arg(QStringLiteral("连接失败")));
        ui->QueryInfo->setEnabled(false);
        ui->DeleteInfo->setEnabled(false);
    }
    else if (odbc_status == _SUCCESS_STATUS)
    {
        ui->db_state_label->setText(tr("<font style='color:#01847e; font:bold;'>%1</font>").arg(QStringLiteral("连接成功")));
        if (ui->TheSerialNo->text().length())
        {
            ui->QueryInfo->setEnabled(true);
            ui->DeleteInfo->setEnabled(true);
        }
    }
    else if (odbc_status == _INVALID_PARA)
    {
        ui->db_state_label->setText(tr("<font style='color:red; font:bold;'>%1</font>").arg(QStringLiteral("信息不正确")));
        ui->QueryInfo->setEnabled(false);
        ui->DeleteInfo->setEnabled(false);
    }
}

void MainDialog::slotConnected0()
{
    server_status[0] = _SUCCESS_STATUS;
}

void MainDialog::slotConnected1()
{
    server_status[1] = _SUCCESS_STATUS;
}

void MainDialog::slotConnected2()
{
    server_status[2] = _SUCCESS_STATUS;
}

void MainDialog::result_Received()
{
    BoothState state;
    qint64 length;

    while (tcpSocket[2]->bytesAvailable() > 0)
    {
        QByteArray datagram;
        length = tcpSocket[2]->bytesAvailable();

        datagram.resize(length);
        tcpSocket[2]->read(datagram.data(), datagram.size());
        memcpy(&state, datagram.data(), length);

        if (state.resp.cmd == RE_HEARTBEAT_SIGNAL)
        {
            //收到治具发来的回应，正常，记下时间戳
//            qDebug() << "recv response from fixture";
        }
        else if (state.resp.cmd == OP_TRIGGER_OUT)
        {
//            qDebug() << "trigger out from fixture";
            ui->booth1state->clear();
            ui->booth2state->clear();
            ui->booth3state->clear();
            ui->booth4state->clear();
            ui->serialno1->clear();
            ui->serialno2->clear();
            ui->serialno3->clear();
            ui->serialno4->clear();
//            need_send_info = true;
        }
        else if (state.resp.cmd == OP_WRITE_BULK_INFO)
        {
//            qDebug() << "write bulk chip data result recved";
            //收到治具批量写数据结果信息
            if (state.state[0] == _CHIP_WRITE_SUCCESS && strlen(state.serial_no[0]) > 8)
            {
                ui->booth1state->setText(tr("<font style='color:#01847e; font:bold;'>%1</font>").arg(QStringLiteral("写入成功")));
                ui->serialno1->setText(tr("<font style='color:#003153;'>SN：%1</font>").arg(state.serial_no[0]));
                //将耗材信息存入数据库
                insert_info_mysql(ui->Chip1SerialNo->text().toLocal8Bit().data());
            }
            else if (state.state[0] == _CHIP_WRITE_FAILED)
                ui->booth1state->setText(tr("<font style='color:#DC143C; font:bold;'>%1</font>").arg(QStringLiteral("写入失败")));
            else if (state.state[0] == _CHIP_HAS_DATA)
                ui->booth1state->setText(tr("<font style='color:#FF6347; font:bold;'>%1</font>").arg(QStringLiteral("芯片已有数据")));
            else if (state.state[0] == _CHIP_MISSING)
                ui->booth1state->setText(tr("<font style='color:#FA8072; font:bold;'>%1</font>").arg(QStringLiteral("未发现芯片")));

            if (state.state[1] == _CHIP_WRITE_SUCCESS && strlen(state.serial_no[1]) > 8)
            {
                ui->booth2state->setText(tr("<font style='color:#01847e; font:bold;'>%1</font>").arg(QStringLiteral("写入成功")));
                ui->serialno2->setText(tr("<font style='color:#003153;'>SN：%1</font>").arg(state.serial_no[1]));
                insert_info_mysql(ui->Chip2SerialNo->text().toLocal8Bit().data());
            }
            else if (state.state[1] == _CHIP_WRITE_FAILED)
                ui->booth2state->setText(tr("<font style='color:#DC143C; font:bold;'>%1</font>").arg(QStringLiteral("写入失败")));
            else if (state.state[1] == _CHIP_HAS_DATA)
                ui->booth2state->setText(tr("<font style='color:#FF6347; font:bold;'>%1</font>").arg(QStringLiteral("芯片已有数据")));
            else if (state.state[1] == _CHIP_MISSING)
                ui->booth2state->setText(tr("<font style='color:#FA8072; font:bold;'>%1</font>").arg(QStringLiteral("未发现芯片")));

            if (state.state[2] == _CHIP_WRITE_SUCCESS && strlen(state.serial_no[2]) > 8)
            {
                ui->booth3state->setText(tr("<font style='color:#01847e; font:bold;'>%1</font>").arg(QStringLiteral("写入成功")));
                ui->serialno3->setText(tr("<font style='color:#003153;'>SN：%1</font>").arg(state.serial_no[2]));
                insert_info_mysql(ui->Chip3SerialNo->text().toLocal8Bit().data());
            }
            else if (state.state[2] == _CHIP_WRITE_FAILED)
                ui->booth3state->setText(tr("<font style='color:#DC143C; font:bold;'>%1</font>").arg(QStringLiteral("写入失败")));
            else if (state.state[2] == _CHIP_HAS_DATA)
                ui->booth3state->setText(tr("<font style='color:#FF6347; font:bold;'>%1</font>").arg(QStringLiteral("芯片已有数据")));
            else if (state.state[2] == _CHIP_MISSING)
                ui->booth3state->setText(tr("<font style='color:#FA8072; font:bold;'>%1</font>").arg(QStringLiteral("未发现芯片")));

            if (state.state[3] == _CHIP_WRITE_SUCCESS && strlen(state.serial_no[3]) > 8)
            {
                ui->booth4state->setText(tr("<font style='color:#01847e; font:bold;'>%1</font>").arg(QStringLiteral("写入成功")));
                ui->serialno4->setText(tr("<font style='color:#003153;'>SN：%1</font>").arg(state.serial_no[3]));
                insert_info_mysql(ui->Chip4SerialNo->text().toLocal8Bit().data());
            }
            else if (state.state[3] == _CHIP_WRITE_FAILED)
                ui->booth4state->setText(tr("<font style='color:#DC143C; font:bold;'>%1</font>").arg(QStringLiteral("写入失败")));
            else if (state.state[3] == _CHIP_HAS_DATA)
                ui->booth4state->setText(tr("<font style='color:#FF6347; font:bold;'>%1</font>").arg(QStringLiteral("芯片已有数据")));
            else if (state.state[3] == _CHIP_MISSING)
                ui->booth4state->setText(tr("<font style='color:#FA8072; font:bold;'>%1</font>").arg(QStringLiteral("未发现芯片")));

            int remain_num = ui->QuantityRemain->text().toInt();
            current_number = serial_id;
//            qDebug() << __func__ << remain_num;
            if (remain_num <= 4)
            {
                //任务全部完成
                is_done = true;
                this->clear_serialno_info();
                ui->QuantityRemain->setNum(0);
                ui->CurrentNumber->setNum(-1);

                QMessageBox::information(this, tr("耗材芯片烧录完成"),
                            tr("%1，你好：\n恭喜你，完成本次耗材烧录任务，辛苦了！\n\n本次任务信息：\n组件料号：%2\n烧录总数：%3\n")
                            .arg(ui->OperatorID->text()).arg(ui->ComponentNo->text()).arg(ui->PlannedNumber->text()));
                return;
            }
            else
            {
                ui->QuantityRemain->setNum(remain_num - 4);
                ui->CurrentNumber->setNum(current_number);
            }

            if (remain_num - 4 > 4)
                update_serialno(4);
            else
                update_serialno(remain_num - 4);
        }
    }
}

bool MainDialog::open_longconn_socket()
{
    MsgHdr hdr;

    if (tcpSocket[2] == NULL)
        tcpSocket[2] = new QTcpSocket(this);

    connect(tcpSocket[2], SIGNAL(connected()), this, SLOT(slotConnected2()));
    connect(tcpSocket[2], SIGNAL(readyRead()), this, SLOT(result_Received()));

    if (tcpSocket[2]->isOpen())
        tcpSocket[2]->close();

    server_status[2] = _FAILED_STATUS;
    tcpSocket[2]->connectToHost(ui->FixtureIPAddr->text(), TCP_PORT);
    Sleep(100);
    if (server_status[2] == _FAILED_STATUS)
        return _FAILED_STATUS;

    hdr.cmd = OP_GET_STATE_LONGCONN;
    hdr.len = 0;

    if (tcpSocket[2]->write((const char*)&hdr, sizeof(hdr)) == -1)
        return _FAILED_STATUS;

    return _SUCCESS_STATUS;
}

//定时发送检测治具状态信息
//返回false表示离线，返回true表示在线
bool MainDialog::check_server_status()
{
    if (tcpSocket[0] != NULL)
        delete tcpSocket[0];
    tcpSocket[0] = new QTcpSocket(this);

    connect(tcpSocket[0], SIGNAL(connected()), this, SLOT(slotConnected0()));

    server_status[0] = _FAILED_STATUS;
    tcpSocket[0]->connectToHost(ui->FixtureIPAddr->text(), TCP_PORT);

    Sleep(100);
    return server_status[0];
}

void MainDialog::on_FixtureIPAddr_textChanged(const QString &arg1)
{
    Q_UNUSED(arg1);
//    qDebug() << __func__ << ui->FixtureIPAddr->text();

    if (ui->FixtureIPAddr->text().length() &&
        checkIpValid(checkIPversion(ui->FixtureIPAddr->text()), ui->FixtureIPAddr->text()))
    {
        if (tcpSocket[0])
        {
            if (tcpSocket[0]->isOpen())
                tcpSocket[0]->close();
            delete tcpSocket[0];
            tcpSocket[0] = NULL;
        }
        if (tcpSocket[1])
        {
            if (tcpSocket[1]->isOpen())
                tcpSocket[1]->close();
            delete tcpSocket[1];
            tcpSocket[1] = NULL;
        }
        if (tcpSocket[2])
        {
            if (tcpSocket[2]->isOpen())
                tcpSocket[2]->close();
            delete tcpSocket[2];
            tcpSocket[2] = NULL;
        }
    }
    else if (ui->FixtureIPAddr->text().length())
    {
//        qDebug() << "invalid ip addr";
        ui->fixture_state_label->setText("<p style=\"color:red;font-weight:bold\">IP 地址不正确！</p>");
        ui->booth1state->setText(tr("<font style='color:FF0000; font:bold;'>%1</font>").arg(QStringLiteral("---------")));
        ui->booth2state->setText(tr("<font style='color:FF0000; font:bold;'>%1</font>").arg(QStringLiteral("---------")));
        ui->booth3state->setText(tr("<font style='color:FF0000; font:bold;'>%1</font>").arg(QStringLiteral("---------")));
        ui->booth4state->setText(tr("<font style='color:FF0000; font:bold;'>%1</font>").arg(QStringLiteral("---------")));
    }
    else
    {
//        qDebug() << "FixtureIPAddr is empty!";
        ui->fixture_state_label->clear();

        ui->booth1state->clear();
        ui->booth2state->clear();
        ui->booth3state->clear();
        ui->booth4state->clear();
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
        qDebug()<<"error open database because"<<db.lastError().text();
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
    else if (ui->DBIPAddr->text().length() == 0 &&
             ui->DBUser->text().length() == 0 &&
             ui->DBPasswd->text().length() == 0 &&
             ui->DBSource->text().length() == 0)
    {
        odbc_status = _INVALID_PARA;
        ui->db_state_label->clear();
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

void MainDialog::set_style_sheet(QString filename)
{
    QString qssfile = QString(":/qss/%1").arg(filename);
    QFile skinfile(qssfile);
    skinfile.open(QIODevice::ReadOnly);
    this->setStyleSheet(skinfile.readAll());
    skinfile.close();
}

void MainDialog::bulkdataReceived()
{

}

//发送读写芯片数据
bool MainDialog::sendData(int cmd, void* data, int data_len)
{
    uint8_t writeinfo[sizeof(MsgHdr) + data_len];

    if (tcpSocket[1] == NULL)
        tcpSocket[1] = new QTcpSocket(this);

    connect(tcpSocket[1], SIGNAL(connected()), this, SLOT(slotConnected1()));
    connect(tcpSocket[1], SIGNAL(readyRead()), this, SLOT(bulkdataReceived()));

    if (tcpSocket[1]->isOpen())
        tcpSocket[1]->close();

    server_status[1] = _FAILED_STATUS;
    tcpSocket[1]->connectToHost(ui->FixtureIPAddr->text(), TCP_PORT);
    Sleep(100);
    if (server_status[1] == _FAILED_STATUS)
        return false;

    ((MsgHdr*)writeinfo)->cmd = cmd;
    ((MsgHdr*)writeinfo)->len = data_len;
    if (cmd == OP_SEND_BULK_INFO)
    {
        if (strncasecmp(((BoothSupplyInfoW*)data)->model_id, "TL", 2) == 0)
            ((MsgHdr*)writeinfo)->i2c_addr = _TONER_CHIP_ADDR;
        else
            ((MsgHdr*)writeinfo)->i2c_addr = _DRUM_CHIP_ADDR;

        memcpy(writeinfo + sizeof(MsgHdr), data, data_len);
//        this->hex_dump((unsigned char*)&writeinfo, sizeof(writeinfo));
    }

    if (tcpSocket[1]->write((const char*)&writeinfo, sizeof(writeinfo)) == -1)
    {
//        qDebug() << "write to tcp socket failed";
        return _FAILED_STATUS;
    }

    return _SUCCESS_STATUS;
}

void MainDialog::insert_info_mysql(char* serialno)
{
    //将耗材信息存入数据库
    int ret;

    if (odbc_status != _SUCCESS_STATUS)
        return;

    open_sql_server();
    query = QSqlQuery(this->db);
    QString new_sql = QString("insert into %1.%2 (foreman,operator,model_id,serial_no,marketing_area,year,month,day,"
                              "manufacturer,trade_mark,type,pages,dots,overflow_pages,overflow_percent,free_pages)"
                              "values(:foreman:operator,:model_id,:serial_no,:marketing_area,:year,:month,:day,"
                              ":manufacturer,:trade_mark,:type,:pages,:dots,:overflow_pages,"
                              ":overflow_percent,:free_pages)").arg(DATABASE_NAME).arg(TABLE_NAME);

    query.prepare(new_sql);
    query.bindValue(":foreman", ui->ForemanName->text().toLocal8Bit().data());
    query.bindValue(":operator", ui->OperatorID->text().toLocal8Bit().data());
    query.bindValue(":model_id", ui->ChipModelID->text().toLocal8Bit().data());
    query.bindValue(":serial_no", serialno);
    query.bindValue(":marketing_area", ui->MarketingArea->text().toLocal8Bit().data());
    query.bindValue(":year", this->year);
    query.bindValue(":month", this->month);
    query.bindValue(":day", this->day);
    query.bindValue(":manufacturer", ui->manufacturer->currentText().toLocal8Bit().data());
    query.bindValue(":trade_mark", ui->TradeMark->currentText().toLocal8Bit().data());
    query.bindValue(":type", ui->ChipType->text().toLocal8Bit().data());
    query.bindValue(":pages", ui->TotalPages->text().toInt());
    query.bindValue(":dots", 0);
    query.bindValue(":overflow_pages", ui->BeyondPages->text().toInt());
    query.bindValue(":overflow_percent", 0);
    query.bindValue(":free_pages", ui->FreePages->text().toInt());

    ret = query.exec();
    if (ret)
    {
        play_mp3_sound(QCoreApplication::applicationDirPath() + "/sound/done.mp3");
        ui->db_state_label->setText("<p style=\"color:green;font-weight:bold\">写入成功！</p>");
        odbc_status = _INVALID_PARA;
    }
    else
    {
        play_mp3_sound(QCoreApplication::applicationDirPath() + "/sound/failed.mp3");
        ui->db_state_label->setText("<p style=\"color:red;font-weight:bold\">写入失败，可能已有该数据！</p>");
        qDebug() << query.lastError().text() << QString(QObject::tr("写入失败"));
        odbc_status = _INVALID_PARA;
    }
}

void MainDialog::on_QueryInfo_clicked()
{
    if (odbc_status != _SUCCESS_STATUS)
        return;

    int num = 0;
    open_sql_server();

    struct cgprintech_supply_sqlinfo ChipInfo;
    QString sqlcmd = QString("select model_id,serial_no,marketing_area,year,month,day,manufacturer,trade_mark,"
                             "type,pages,overflow_pages,free_pages,operator,foreman from %1.%2 "
                             "where serial_no='%3'").arg(DATABASE_NAME).arg(TABLE_NAME).arg(ui->TheSerialNo->text());
//    qDebug() << sqlcmd;
    query = QSqlQuery(this->db);
    if (!query.exec(sqlcmd))
    {
        qDebug() << query.lastError().driverText() << QString(QObject::tr("读取失败"));
        return;
    }
    else
    {
        while (query.next())
        {
            num++;

            memcpy(ChipInfo.model_id, query.value(0).toString().toLatin1().data(), 16);
            memcpy(ChipInfo.serial_no, query.value(1).toString().toLatin1().data(), 32);
            memcpy(ChipInfo.marketing_area, query.value(2).toString().toLatin1().data(), 4);
            QString date = query.value(3).toString() + "." + query.value(4).toString() + "." + query.value(5).toString();
            memcpy(ChipInfo.product_date, date.toLatin1().data(), 12);
            memcpy(ChipInfo.manufacturer, query.value(6).toString().toLatin1().data(), 16);
            memcpy(ChipInfo.trade_mark, query.value(7).toString().toLatin1().data(), 16);
            memcpy(ChipInfo.type, query.value(8).toString().toLatin1().data(), 4);
            memcpy(ChipInfo.pages, query.value(9).toString().toLatin1().data(), 12);
            memcpy(ChipInfo.overflow_pages, query.value(10).toString().toLatin1().data(), 4);
            memcpy(ChipInfo.free_pages, query.value(11).toString().toLatin1().data(), 4);
            ChipInfo.operator_id = query.value(12).toString();
            ChipInfo.foreman = query.value(13).toString();
//            qDebug() << query.value(12).toString();
        }
    }

    if (num == 0)
    {
        play_mp3_sound(QCoreApplication::applicationDirPath() + "/sound/failed.mp3");
        ui->db_state_label->setText("<p style=\"color:red;font-weight:bold\">查询数据失败！</p>");
        odbc_status = _INVALID_PARA;
        return;
    }

    play_mp3_sound(QCoreApplication::applicationDirPath() + "/sound/done.mp3");
    ui->db_state_label->setText("<p style=\"color:green;font-weight:bold\">查询数据成功！</p>");
    odbc_status = _INVALID_PARA;

    //根据序列号查询数据库中耗材信息
    SqlChipInfo* sqlinfo = new SqlChipInfo();

    connect(this, SIGNAL(sendThemeMode(int)), sqlinfo, SLOT(get_theme_id(int)));
    connect(this, SIGNAL(sendSqlInfo(struct cgprintech_supply_sqlinfo*)),
            sqlinfo, SLOT(recvSqlInfo(struct cgprintech_supply_sqlinfo*)));

    emit sendThemeMode(theme_state);
    emit sendSqlInfo(&ChipInfo);

    sqlinfo->show();
}

void MainDialog::on_DeleteInfo_clicked()
{
    //根据序列号删除数据库中耗材记录
    bool ret;

    if (odbc_status != _SUCCESS_STATUS || ui->TheSerialNo->text().length())
        return;

    QString sqlcmd = QString("delete from %1.%2 where serial_no='%3'").arg(DATABASE_NAME).arg(TABLE_NAME).arg(ui->TheSerialNo->text());
    qDebug() << sqlcmd;
    query = QSqlQuery(this->db);
    ret = query.exec(sqlcmd);

    if (ret)
        ui->db_state_label->setText("<p style=\"color:green;font-weight:bold\">删除数据库记录成功！</p>");
    else
        ui->db_state_label->setText("<p style=\"color:red;font-weight:bold\">删除数据库记录失败！</p>");
}

int MainDialog::StringToHex(char *str, unsigned char *out, unsigned int *outlen)
{
    char *p = str;
    char high = 0, low = 0;
    int tmplen = strlen(p), cnt = 0;
    tmplen = strlen(p);

    while (cnt < (tmplen / 2))
    {
        high =  ((*p > '9') && ((*p <= 'F') || (*p <= 'f'))) ? *p - 48 - 7 : *p - 48;
        low = (*(++p) > '9' && ((*p <= 'F') || (*p <= 'f'))) ? *(p) - 48 - 7 : *(p) - 48;
        out[cnt] = ((high & 0x0f) << 4 | (low & 0x0f));
        p++;
        cnt++;
    }

    if (tmplen % 2 != 0)
        out[cnt] = ((*p > '9') && ((*p <= 'F') || (*p <= 'f'))) ? *p - 48 - 7 : *p - 48;

    if (outlen != NULL)
        *outlen = tmplen / 2 + tmplen % 2;

    return tmplen / 2 + tmplen % 2;
}

void MainDialog::Pack16(unsigned char *dst, unsigned int val)
{
    dst[0] = (unsigned char)((val >> 8) & 0xff);
    dst[1] = (unsigned char)(val & 0xff);
}

void MainDialog::Pack32(unsigned char *dst, unsigned int val)
{
    dst[0] = (unsigned char)((val >> 24) & 0xff);
    dst[1] = (unsigned char)((val >> 16) & 0xff);
    dst[2] = (unsigned char)((val >> 8) & 0xff);
    dst[3] = (unsigned char)(val & 0xff);
}

unsigned int MainDialog::Unpack16(unsigned char *src)
{
    return (((unsigned int)src[0]) << 8
          | ((unsigned int)src[1]));
}

unsigned int MainDialog::Unpack32(unsigned char *src)
{
    return(((unsigned int)src[0]) << 24
         | ((unsigned int)src[1]) << 16
         | ((unsigned int)src[2]) << 8
         | (unsigned int)src[3]);
}

void MainDialog::hex_dump(const unsigned char *src, size_t length)
{
    int i = 0;
    const unsigned char *address = src;
    unsigned int num=0;
    size_t line_size=16;

    printf("%08x | ", num);
    num += 16;
    while (length-- > 0)
    {
        printf("%02x ", *address++);

        if ((i+1)%8==0 && (i+1)%16==8)
        {
            printf("  ");
        }

        if (!(++i % line_size) || (length == 0 && i % line_size))
        {
            printf("\n");

            if (length > 0)
            {
                printf("%08x | ", num);
                num += 16;
            }
        }
    }
    printf("\n");
}

void MainDialog::on_TheSerialNo_textChanged(const QString &arg1)
{
    Q_UNUSED(arg1);

    if (ui->TheSerialNo->text().length())
    {
        if (odbc_status == _SUCCESS_STATUS)
        {
            ui->QueryInfo->setEnabled(true);
            ui->DeleteInfo->setEnabled(true);
        }
    }
    else
    {
        ui->QueryInfo->setEnabled(false);
        ui->DeleteInfo->setEnabled(false);
    }
}

void MainDialog::on_RecommendFixtures_itemDoubleClicked(QListWidgetItem *item)
{
    //双击后，将该行IP填入治具IP地址栏
    ui->FixtureIPAddr->setText(item->text());
}
