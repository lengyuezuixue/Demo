#ifndef YOLO_CLIENT_H_1022
#define YOLO_CLIENT_H_1022

#include <QObject>
#include <QAbstractSocket>


class QTcpSocket;
class YoloClient : public QObject
{
	Q_OBJECT

public:
	static YoloClient *instance();

	void setPort(int port);
	void connectServer();

    void sendData(const QByteArray &array);

    bool isConnected();

signals:
    void newImageAvailabled(const QImage &image);

private slots:
	void onReadyRead();
	void onConnected();
	void onDisconnected();
	void onError(QAbstractSocket::SocketError socketError);
	void onStateChanged(QAbstractSocket::SocketState socketState);
	
private:
	YoloClient(QObject *parent = nullptr);
	~YoloClient();

private:
	int  m_port;
	QTcpSocket *m_pSocket;

    QByteArray m_buffer; //缓存上一次或多次的未处理的数据, 这个用来处理，重新粘包
};

#define YOLO_CLIENT	(YoloClient::instance())

#endif


