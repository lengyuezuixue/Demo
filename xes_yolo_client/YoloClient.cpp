#include <QTcpSocket>
#include <QDataStream>
#include <QImage>
#include <QPixmap>
#include <QDebug>

#include "YoloClient.h"

YoloClient::YoloClient(QObject *parent)
	: QObject(parent)
	, m_port(55920)
    , m_recvDataSize(0)
    , m_dataSize(0)
{
	m_pSocket = new QTcpSocket();

    m_pixArray.resize(0);

    connect(m_pSocket, SIGNAL(connected()), this, SLOT(onConnected()));
    connect(m_pSocket, SIGNAL(disconnected()), this, SLOT(onDisconnected()));
	connect(m_pSocket, SIGNAL(error(QAbstractSocket::SocketError )), 
		this, SLOT(onError(QAbstractSocket::SocketError)));
	connect(m_pSocket, SIGNAL(stateChanged(QAbstractSocket::SocketState)), 
		this, SLOT(onStateChanged(QAbstractSocket::SocketState)));

	connect(m_pSocket, SIGNAL(readyRead()), this, SLOT(onReadyRead()));
}

YoloClient::~YoloClient()
{
	if (m_pSocket) {
		m_pSocket->close();

		delete m_pSocket;
        m_pSocket = nullptr;
	}
}


YoloClient *YoloClient::instance()
{
	static YoloClient _instance;
	return &_instance;
}

void YoloClient::setPort(int port)
{
	m_port = port;
}

void YoloClient::connectServer()
{
    m_pSocket->connectToHost(QString("127.0.0.1"), (quint16)m_port);
}

void YoloClient::sendData(const QByteArray &array)
{
    if (m_pSocket && m_pSocket->state() == QAbstractSocket::ConnectedState) {
        m_pSocket->write(array);
    }
}

void YoloClient::onConnected()
{
    qDebug() << Q_FUNC_INFO;
}

void YoloClient::onDisconnected()
{
    qDebug() << Q_FUNC_INFO;
}

void YoloClient::onError(QAbstractSocket::SocketError socketError)
{
    qDebug() << Q_FUNC_INFO << socketError;
}

void YoloClient::onStateChanged(QAbstractSocket::SocketState socketState)
{
    qDebug() << Q_FUNC_INFO << socketState;
}

void YoloClient::onReadyRead()
{
	QTcpSocket *obj = dynamic_cast<QTcpSocket*>(sender());
    if (obj == nullptr) {
		return;
	}

   QByteArray msg = obj->readAll();

   if (m_recvDataSize == 0){
       QDataStream out(msg);

       int size;
       out >> size >> size >> size >> size >> size;

       qDebug() << "size:" << size;

       m_dataSize = size + 20;
   }

   if (m_recvDataSize != m_dataSize){

       m_recvDataSize += msg.size();
       m_pixArray.append(msg);
   }


   if (m_recvDataSize == m_dataSize && m_recvDataSize != 0){
        m_recvDataSize = 0;
        m_dataSize = 0;

        QDataStream out(m_pixArray);
        int flag, width, heigth, rgb, size;
        out >> flag >> width >> heigth >> rgb >> size;

        qDebug() << Q_FUNC_INFO << QString("flag:%1, width:%2, heigth:%3, rgb:%4, size:%5").arg(flag).arg(width).arg(heigth).arg(rgb).arg(size);


        QByteArray pixArray = m_pixArray.right(size);


        QImage img;
        img.loadFromData(pixArray);

        if (!img.isNull()){
            emit newImageAvailabled(img);
        }
        else {
            qDebug() << "img is NULL";
        }


        m_pixArray.resize(0);
   }
}
