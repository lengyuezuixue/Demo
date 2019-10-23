#include <QTcpSocket>
#include <QImage>
#include <QDebug>
#include <QBuffer>
#include <QPainter>
#include <QApplication>

#include "mainwidget.h"
#include "ui_mainwidget.h"

#include "YoloClient.h"
#include "YoloServerManager.h"

MainWidget::MainWidget(QWidget *parent)
	: QWidget(parent)
	, ui(new Ui::MainWidget)
{
	ui->setupUi(this);


    YOLO_SERVER_MRG->start();


    connect(YOLO_CLIENT, SIGNAL(newImageAvailabled(QImage)), this, SLOT(onNewImageAvailabled(QImage)));
}


void MainWidget::testImage()
{
    QPixmap pix("E:\\yolo\\image\\ppt1.png");

    QBuffer buffer;
    pix.save(&buffer, "png");

    int flag = 1;
    int rgb = 3;


    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly );
    stream.setVersion(QDataStream::Qt_5_12);
    stream << flag;
    stream<< pix.width();
    stream << pix.height();
    stream << rgb;
    stream << buffer.data().size();
    data.append( buffer.data());

    qDebug()<<"img:"<<data.size()<<"buffer"<<buffer.data().size();

    qDebug() << Q_FUNC_INFO << data;

    YOLO_CLIENT->sendData(data);

}

void MainWidget::onNewImageAvailabled(const QImage &img)
{
    m_pix = QPixmap::fromImage(img);

    update();
}

void MainWidget::on_pushButton_clicked()
{
    testImage();
}

void MainWidget::paintEvent(QPaintEvent *ev)
{
    Q_UNUSED(ev);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    if (m_pix.isNull()){
        return ;
    }

    QPixmap pix = m_pix.scaled(size());
    painter.drawPixmap(rect(), pix);
}
