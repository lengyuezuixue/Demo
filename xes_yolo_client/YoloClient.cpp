#include <QTcpSocket>
#include <QDataStream>
#include <QImage>
#include <QPixmap>
#include <QDebug>

#include "YoloClient.h"

#define PACKAGE_HEAD_SIZE  20

YoloClient::YoloClient(QObject *parent)
	: QObject(parent)
	, m_port(55920)
{
	m_pSocket = new QTcpSocket();


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

bool YoloClient::isConnected()
{
    return m_pSocket->state() == QAbstractSocket::ConnectedState;
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
    if (obj == nullptr || obj->bytesAvailable() <= 0) {
		return;
	}

    QByteArray buffer = obj->readAll();
    m_buffer.append(buffer);

    int packageSize = 0;
    int flag, width, heigth, rgb, size;

    int totalLen = m_buffer.size();
    while (totalLen){
        if (totalLen < PACKAGE_HEAD_SIZE){ //不够包头的数据直接就不处理
            break;
        }

        QDataStream packet(m_buffer);
        packet.setByteOrder(QDataStream::LittleEndian);

        packet >> flag >> width >> heigth >> rgb >> size;

        packageSize = size + PACKAGE_HEAD_SIZE;

        //如果不够长度等够了在来解析
        if (totalLen < packageSize){
            break;
        }

        if (flag == 0){
            QByteArray pixArray = m_buffer.mid(PACKAGE_HEAD_SIZE, size);

            QImage img;
            img.loadFromData(pixArray);

            emit newImageAvailabled(img);
        }


        //缓存多余的数据
        buffer = m_buffer.right(totalLen - packageSize);

        //更新长度
        totalLen = buffer.size();

        //更新多余数据
        m_buffer = buffer;
    }
}
