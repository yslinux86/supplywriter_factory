#include "statemonitor.h"
#include "maindialog.h"

#include <QDebug>
#include <QThread>
#include <QTime>
#include <QSqlError>
#include <QCoreApplication>

StateMonitor::StateMonitor()
{
    db_thread = QSqlDatabase::addDatabase("QODBC", "thread");
}

void StateMonitor::run()
{
    while(1)
    {
        if (this->checkIpValid(this->checkIPversion(db_ip), db_ip) &&
            db_user.length() &&
            db_pwd.length() &&
            db_ds.length())
        {
            this->open_sql_server();
            emit DB_Connect_Signal(odbc_status);
        }
        else
        {
            emit DB_Connect_Signal(_INVALID_PARA);
        }

        this->sleep(3);
    }
}

void StateMonitor::on_get_sql_config(QString _db_ip, QString _db_user, QString _db_pwd, QString _db_ds)
{
    db_ip = _db_ip;
    db_user = _db_user;
    db_pwd = _db_pwd;
    db_ds = _db_ds;
}

void StateMonitor::open_sql_server()
{
    if (db_thread.isOpen())
        db_thread.close();

    db_thread.setHostName(db_ip);
    db_thread.setUserName(db_user);
    db_thread.setPassword(db_pwd);
    db_thread.setDatabaseName(db_ds);   //这里填写数据源名称

    db_thread.setConnectOptions("SQL_ATTR_LOGIN_TIMEOUT=1");
    if (db_thread.open(db_user, db_pwd))
    {
//        qDebug() << "open success";
        odbc_status = _SUCCESS_STATUS;
    }
    else
    {
        qDebug() << "error open database because" << db_thread.lastError().text();
        odbc_status = _FAILED_STATUS;
    }
}

//检查IP地址是否合法
bool StateMonitor::checkIpValid(int version, QString ip)
{
    if (version == 4)
    {
        QRegExp rx1("^((25[0-5]|2[0-4]\\d|[01]?\\d\\d?)\\.){3}(25[0-5]|2[0-4]\\d|[01]?\\d\\d?)$");
        if(rx1.exactMatch(ip))
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
int StateMonitor::checkIPversion(QString IP)
{
    if (IP.contains(":", Qt::CaseInsensitive))
        return 6;
    else if (IP.contains(".", Qt::CaseInsensitive))
        return 4;

    return 0;
}

void StateMonitor::Sleep(int msec)
{
    QTime dieTime = QTime::currentTime().addMSecs(msec);

    while(QTime::currentTime() < dieTime)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
}
