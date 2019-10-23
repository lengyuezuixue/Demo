#ifndef YOLO_SERVER_MANAGER_H_1023
#define YOLO_SERVER_MANAGER_H_1023

#include <QObject>
#include <QProcess>

class YoloServerManager : public QObject
{
    Q_OBJECT
public:
    static YoloServerManager *instance();

    void stop();
    bool isRuning();

public slots:
    void start();


private slots:
    void onReadyStandarOutput();
    void onReadyStandarError();
    void onFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onErrorOccurred(QProcess::ProcessError error);
    void onStateChanged(QProcess::ProcessState newState);

    void onDelayStartClient();

private:
    YoloServerManager(QObject *parent = nullptr);
    ~YoloServerManager();

    bool taskkill(const QString &processName);
    QString getProcessName();

private:
    QProcess    *m_process;

    QString m_path;
    int m_listenPort;
};

#define YOLO_SERVER_MRG (YoloServerManager::instance())

#endif // YOLOSERVERMANAGER_H
