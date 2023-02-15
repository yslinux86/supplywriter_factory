#include <QDebug>

#include "sqlchipinfo.h"
#include "ui_sqlchipinfo.h"
#include "maindialog.h"

SqlChipInfo::SqlChipInfo(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SqlChipInfo)
{
    ui->setupUi(this);

    setWindowFlags(Qt::WindowMinimizeButtonHint |
                   Qt::WindowMaximizeButtonHint |
                   Qt::WindowCloseButtonHint);

    ui->closebutton->setDefault(true);
    ui->chipserialno->setReadOnly(true);
}

SqlChipInfo::~SqlChipInfo()
{
    delete ui;
}

void SqlChipInfo::set_style_sheet(QString filename)
{
    QString qssfile = QString(":/qss/%1").arg(filename);
    QFile skinfile(qssfile);
    skinfile.open(QIODevice::ReadOnly);
    this->setStyleSheet(skinfile.readAll());
    skinfile.close();
}

void SqlChipInfo::get_theme_id(int state)
{
    if (state == 2)
    {
        this->set_style_sheet("dark.qss");
    }
    else if (state == 0)
    {
        this->set_style_sheet("light.qss");
    }
}

void SqlChipInfo::recvSqlInfo(struct cgprintech_supply_sqlinfo* ChipInfo)
{
    ui->chipmodel->setText(ChipInfo->model_id);
    ui->chipserialno->setText(ChipInfo->serial_no);
    ui->marketingarea->setText(ChipInfo->marketing_area);
    ui->productdate->setText(ChipInfo->product_date);
    ui->pages->setText(ChipInfo->pages);
    ui->manufacturer->setText(ChipInfo->manufacturer);
    ui->trademark->setText(ChipInfo->trade_mark);
    ui->chiptype->setText(ChipInfo->type);
    ui->beyondpages->setText(ChipInfo->overflow_pages);
    ui->freepages->setText(ChipInfo->free_pages);
    ui->OperatorID->setText(ChipInfo->operator_id);
    ui->ForemanName->setText(ChipInfo->foreman);

    this->setWindowTitle(ChipInfo->serial_no);
}

void SqlChipInfo::mousePressEvent(QMouseEvent *event)
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

void SqlChipInfo::mouseMoveEvent(QMouseEvent *event)
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

void SqlChipInfo::mouseReleaseEvent(QMouseEvent *event)
{
    //放下左键即停止移动
    if (event->button() == Qt::LeftButton)
    {
        is_drag = false;
    }
}

void SqlChipInfo::on_closebutton_clicked()
{
    this->accept();
}
