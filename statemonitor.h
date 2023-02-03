#ifndef STATEMONITOR_H
#define STATEMONITOR_H

#include <QTcpSocket>
#include <QThread>
#include <QSqlDatabase>

#include "common.h"

#define _INVALID_PARA    2

class MainDialog;

class StateMonitor : public QThread
{
    Q_OBJECT

public:
    StateMonitor();

signals:
    void DB_Connect_Signal(quint8 _odbc_status);

private slots:
    void on_get_sql_config(QString _db_ip, QString _db_user, QString _db_pwd, QString _db_ds);

private:
    QString db_ip, db_user, db_pwd, db_ds, serv_ip;
    quint8 odbc_status;
    QSqlDatabase db_thread;
    void open_sql_server();
    bool checkIpValid(int version, QString ip);
    int checkIPversion(QString IP);
    void Sleep(int msec);

protected:
    virtual void run() override;
};

#endif // WORKER_H
