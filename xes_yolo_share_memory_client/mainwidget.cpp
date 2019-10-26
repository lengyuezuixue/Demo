#include <QDebug>
#include <QBuffer>

#include "mainwidget.h"
#include "ui_mainwidget.h"

#define BUF_SIZE  4096


MainWidget::MainWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MainWidget)
{
    ui->setupUi(this);

     m_sharedMemory.setKey("xes_yolo_share");

     m_sysSemaphore = new sm::SystemSemaphore("xes_yolo_ssm", 1, sm::SystemSemaphore::Create);
     m_sysSemaphore->acquire();

}

MainWidget::~MainWidget()
{
    delete ui;
}

void MainWidget::on_pushButton_clicked()
{
    m_sysSemaphore->release();

    if (!m_sharedMemory.isAttached()){
        bool ret = m_sharedMemory.attach();

        if(!ret){
            qDebug() << QString("err:%1, errstring:%2").arg(m_sharedMemory.error()).arg(QString::fromStdString(m_sharedMemory.errorString()));
            return ;
        }

    }

    int size = m_sharedMemory.size();
    qDebug() << "TTTT::" << size;

    m_sharedMemory.lock();

    QByteArray bytearr;
    bytearr.resize(size);
    char * to=bytearr.data();
    char * from=(char *)m_sharedMemory.data();
    memcpy(to,from, (size_t)size);
    QDataStream stream(&bytearr,QIODevice::ReadOnly);
    stream.setByteOrder(QDataStream::LittleEndian);

    int tt;
    stream >> tt;
    qDebug() << "HHHHHHH::  " << tt;


    m_sharedMemory.unlock();
    m_sharedMemory.detach();

}
