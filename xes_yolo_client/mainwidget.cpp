#include <QTcpSocket>
#include <QImage>
#include <QDebug>
#include <QBuffer>
#include <QPainter>
#include <QDateTime>
#include <QApplication>
#include <QTimer>

#include "mainwidget.h"
#include "ui_mainwidget.h"


#include "YoloClient.h"
#include "YoloServerManager.h"

MainWidget::MainWidget(QWidget *parent)
	: QWidget(parent)
	, ui(new Ui::MainWidget)
    , m_uploading(false)
    , m_uploadNum(0)
{
	ui->setupUi(this);

    m_pTimer = new QTimer(this);
    connect(m_pTimer, SIGNAL(timeout()), this, SLOT(onTimeout()));

    //YOLO_SERVER_MRG->start();

    YOLO_CLIENT->connectServer();

    connect(YOLO_CLIENT, SIGNAL(newImageAvailabled(QImage)), this, SLOT(onNewImageAvailabled(QImage)));
}


void MainWidget::testImage()
{
    if (m_uploading){
        return ;
    }

    QPixmap pix("E:\\yolo\\image\\ppt1.png");

    QBuffer buffer;
    pix.save(&buffer, "png");

    int flag = 0;
    int rgb = 3;


    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly );
    stream.setByteOrder(QDataStream::LittleEndian);

    stream << flag;
    stream<< pix.width();
    stream << pix.height();
    stream << rgb;
    stream << buffer.data().size();
    data.append( buffer.data());

    qDebug()<<"img:"<<data.size()<<"buffer"<<buffer.data().size();

    qDebug() << Q_FUNC_INFO << data;

     m_elapsedTimer.start();
     ++m_uploadNum;
     m_uploading = true;


     flag = 1;
     QByteArray data2;
     QDataStream stream2(&data2, QIODevice::WriteOnly );
     stream2.setByteOrder(QDataStream::LittleEndian);
     stream2 << flag;
     stream2<< pix.width();
     stream2 << pix.height();
     stream2 << rgb;
     stream2 << buffer.data().size();
     data2.append( buffer.data());


    YOLO_CLIENT->sendData(data);
    YOLO_CLIENT->sendData(data2);

    qDebug() << "start time" << QDateTime::currentDateTime().toMSecsSinceEpoch();

}

void MainWidget::onTimeout()
{
    testImage();
}

void MainWidget::onNewImageAvailabled(const QImage &img)
{
    m_uploading =false;
   qDebug() << QString("num=%1, use %2 ms").arg(m_uploadNum).arg(m_elapsedTimer.elapsed());
    m_pix = QPixmap::fromImage(img);

    update();
}

void MainWidget::on_pushButton_clicked()
{
    testImage();
     m_pTimer->start(50);
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
