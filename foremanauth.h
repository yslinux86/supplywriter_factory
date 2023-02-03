#ifndef FOREMANAUTH_H
#define FOREMANAUTH_H

#include <QDialog>
#include <QPixmap>
#include <QString>
#include <QSettings>
#include <QRegExpValidator>

namespace Ui {
class ForeManAuth;
}

class ForeManAuth : public QDialog
{
    Q_OBJECT

public:
    explicit ForeManAuth(QWidget *parent = nullptr);
    ~ForeManAuth();

signals:
    void send_work_content(QString operator_name, QString foreman,
                           QString component_id, quint32 planned_number, quint32 first_number);

private slots:
    void get_operator_name(QString name);
    void on_OKButton_clicked();
    void on_CancelButton_clicked();

protected:
    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);

private:
    Ui::ForeManAuth *ui;
    bool is_drag = false;
    QPoint mouse_start_point;
    QPoint window_start_point;
    QPixmap pixmap;
    QString operator_name;
    QSettings setting;
    QRegExpValidator* val;

private:
    void init_dialog();
};

#endif // FOREMANAUTH_H
