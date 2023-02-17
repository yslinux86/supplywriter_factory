#ifndef MAINDIALOG_H
#define MAINDIALOG_H

#include <QDialog>
#include <QMouseEvent>
#include <QHostAddress>
#include <QTcpSocket>
#include <QUdpSocket>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QMouseEvent>
#include <QSettings>
#include <QPaintEvent>
#include <QImage>
#include <QThread>
#include <QUiLoader>
#include <QListWidgetItem>
#include <QSystemTrayIcon>
#include <QMediaPlayer>
#include <QButtonGroup>
#include <QTimer>
#include <QCheckBox>
#include <QRadioButton>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>

#include "statemonitor.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainDialog; }
QT_END_NAMESPACE

// 服务端监听的端口号
#define TCP_PORT       8899
//监听的广播端口
#define BC_UDP_PORT    8890
#define HB_UDP_PORT    8891

#define _SUCCESS_STATUS       0
#define _FAILED_STATUS        1

#define _AUTO_WRITE_MODE      0
#define _MANUAL_READ_MODE     1

static QString _trademark[] =
{
    "Cgprintech",
    "Lanxum"
};

static QString serial_prefix = "CG";
static QString serial_trade = "CGRX";
static QString serial_year[] =
{
    "A", "B", "C", "D", "F", "G", "H", "J", "K", "N",
    "R", "T", "W", "X", "Y", "L", "P", "Q", "S", "Z",
    "B", "C", "D", "F", "G", "H", "J", "K", "M", "N"
};
static QString serial_month[] =
{
    "1", "2", "3", "4", "5", "6", "7", "8", "9", "A",
    "B", "C"
};
static QString serial_day[] =
{
    "1", "2", "3", "4", "5", "6", "7", "8", "9", "A",
    "B", "C", "D", "E", "F", "G", "H", "I", "J", "K",
    "L", "M", "N", "O", "P", "Q", "R", "S", "T", "U",
    "V"
};

struct supplyinfo
{
    QString componentid;
    QString model;
    QString type;
    QString marketing_area;
    quint32 pages;
    quint16 freepages;
    quint16 beyondpages;
};

static struct supplyinfo info[] =
{
    { "L2100000107", "TL-341L", "I", "CN", 1500,  10, 45 },
    { "0301000259",  "TL-341L", "M", "CN", 1500,  10, 45 },
    { "0301000260",  "TL-341",  "M", "CN", 3000,  10, 90 },
    { "0301000261",  "TL-341H", "M", "CN", 5500,  10, 165 },
    { "0301000224",  "TL-340L", "M", "CN", 1500,  10, 45 },
    { "L2100000022", "TL-340",  "I", "CN", 3000,  10, 90 },
    { "0301000223",  "TL-340",  "M", "CN", 3000,  10, 90 },
    { "0301000225",  "TL-340H", "M", "CN", 5500,  10, 165 },
    { "0301000226",  "TL-340U", "M", "CN", 15000, 10, 450 },
    { "L2100000006", "DL-340",  "I", "CN", 30000, 10, 0 },
    { "0204000131",  "DL-340",  "M", "CN", 30000, 10, 0 },
    { "L2100000136", "DL-341",  "I", "CN", 30000, 10, 0 },
    { "0301000258",  "DL-341",  "M", "CN", 30000, 10, 0 }
};

class MainDialog : public QDialog
{
    Q_OBJECT

public:
    MainDialog(QWidget *parent = nullptr);
    ~MainDialog();
    bool checkIpValid(int version, QString ip);
    int checkIPversion(QString IP);
    void Sleep(int msec);

    int StringToHex(char *str, unsigned char *out, unsigned int *outlen);
    void Pack32(unsigned char* dst, unsigned int val);
    void Pack16(unsigned char* dst, unsigned int val);
    unsigned int Unpack32(unsigned char* src);
    unsigned int Unpack16(unsigned char* src);
    void hex_dump(const unsigned char *src, size_t length);
    quint8 odbc_status = _INVALID_PARA;    //odbc数据库连接
    quint8 server_status[3] = {_INVALID_PARA};  //治具连接
    bool udp_hb_status = false;

private:
    Ui::MainDialog *ui;
    StateMonitor* worker = NULL;
    QTimer* timer[5] = {NULL};
    bool is_drag = false;
    QPoint mouse_start_point;
    QPoint window_start_point;
    bool is_done = false;
//    bool need_send_info = true;
    bool chipmode = false;   // false，4组，true，8组
    int year, month, day;
    int theme_state = 0;
    quint16 serial_id;
    quint16 current_number;
    quint16 max_num;           //本次任务最大序号
    QString serial_no[4];
#define CHECK_STATE_SOCKET  0
#define SEND_DATA_SOCKET    1
#define LONGCONN_SOCKET     2
    QTcpSocket* tcpSocket[3] = {NULL};
    QUdpSocket* udpSocket[2] = {NULL};
    QSqlDatabase db;
#define DATABASE_NAME   "cgprintech"
#define TABLE_NAME      "supplyinfo"
    QSqlQuery query;
    QMediaPlayer *player = NULL;
    bool working_mode = _AUTO_WRITE_MODE;  //默认采用自动写入模式
    QString ComponentNo;

signals:
    void send_db_config(QString _db_ip, QString _db_user, QString _db_pwd, QString _db_ds);
    void sendSqlInfo(struct cgprintech_supply_sqlinfo* info);
    void sendThemeMode(int state);

private:
    void init_dialog();
    bool open_longconn_socket();
    bool check_server_status();
    void play_mp3_sound(QString file);
    void try_connect_db();
    void open_sql_server();
    void insert_info_mysql(char* serialno);
    void clear_serialno_info();
    void Update_FixtureStatus();
    void send_udp_hb_pack(QString ipaddr);
    void set_style_sheet(QString filename);
    bool sendData(int cmd, void* data, int data_len);
    QString calculate_checkcode(QString str);
    void update_timestamp();
    void update_serialno(quint8 num);

protected:
    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);

private slots:
    void send_heartbeat_signal();
    void get_work_content(QString operator_name, QString foreman,
                          QString component_id, quint32 planned_number, quint32 first_number);

    void scan_the_fixtures();
    void check_fixture_online();
    void udp_data_recv();
    void udp_hb_recv();
    void slotConnected0();
    void slotConnected1();
    void slotConnected2();
    void result_Received();
    void bulkdataReceived();
    void slotGetDBStatus(quint8 _odbc_status);
    void update_connect_fixture();
    void update_ui_info();

    void on_DBIPAddr_textChanged(const QString &arg1);
    void on_DBSource_textChanged(const QString &arg1);
    void on_DBUser_textChanged(const QString &arg1);
    void on_DBPasswd_textChanged(const QString &arg1);
    void on_FixtureIPAddr_textChanged(const QString &arg1);

    void on_QueryInfo_clicked();
    void on_DeleteInfo_clicked();
    void on_TheSerialNo_textChanged(const QString &arg1);
    void on_RecommendFixtures_itemDoubleClicked(QListWidgetItem *item);
};

#endif // MAINDIALOG_H
