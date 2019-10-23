#include <QtDebug>
#include <QFileInfo>
#include <QTimer>

#include "YoloClient.h"
#include "YoloServerManager.h"

YoloServerManager::YoloServerManager(QObject *parent)
    :QObject (parent)
{
    m_process = new QProcess(this);

    connect(m_process, SIGNAL(readyReadStandardOutput()), this, SLOT(onReadyStandarOutput()));
    connect(m_process, SIGNAL(readyReadStandardError()), this, SLOT(onReadyStandarError()));
    connect(m_process, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(onFinished(int, QProcess::ExitStatus)));
    connect(m_process, SIGNAL(errorOccurred(QProcess::ProcessError)), this, SLOT(onErrorOccurred(QProcess::ProcessError)));
    connect(m_process, SIGNAL(stateChanged(QProcess::ProcessState)), this, SLOT(onStateChanged(QProcess::ProcessState)));

    m_path = QString("E:/yolo/server/xes_yolo_helper.exe");
}

YoloServerManager::~YoloServerManager()
{
      stop();
}

YoloServerManager *YoloServerManager::instance()
{
    static YoloServerManager _instance;
    return &_instance;
}

bool YoloServerManager::taskkill(const QString &processName)
{
    if (processName.isEmpty()) {
        return false;
    }

    QString command = QString("TASKKILL /IM %1 /F").arg(processName);
    return QProcess::execute(command);

}

QString YoloServerManager::getProcessName()
{
    return m_path.mid(m_path.lastIndexOf("/") + 1);;
}

void YoloServerManager::stop()
{
    taskkill(getProcessName());

    if (m_process->state() == QProcess::Running
            || m_process->state() == QProcess::Starting) {
        m_process->terminate();
        if (!m_process->waitForFinished(1000)) {
            m_process->kill();
        }

        qDebug() << ("[ai_yolo] Stop yolo server.");
    }
}

void YoloServerManager::start()
{
    if (m_path.isEmpty()){
        return ;
    }

    QFileInfo fileInfo(m_path);
    QString workDir = fileInfo.absolutePath();
    m_process->setWorkingDirectory(workDir);


    QStringList args;
    m_process->start(m_path, args);

    bool ret = m_process->waitForStarted();

    QString logStr = QString(" path=%1; args=%2; ret=%3").arg(m_path).arg(args.join(",")).arg(ret);
    qDebug() <<QString("[ai_yolo]Start yolo server. %1").arg(logStr);
}

bool YoloServerManager::isRuning()
{
    return (m_process->state() == QProcess::Running);
}

void YoloServerManager::onReadyStandarOutput()
{
    QProcess *p = static_cast<QProcess *>(sender());
    if (p == nullptr) {
        return;
    }

    QString msg = p->readAllStandardOutput();
    qDebug() << QString("[ai_yolo] StandarOutput:%1").arg(msg.data());

     QStringList msgList = msg.split("\r\n", QString::SkipEmptyParts);
     foreach(QString item, msgList){
         if (item.startsWith("listen port:")){
            int idx = item.indexOf(":");
            m_listenPort = item.mid(idx+1, item.length() - idx - 1).toInt();

            QTimer::singleShot(200, this, SLOT(onDelayStartClient()));
         }
     }
}

void YoloServerManager::onDelayStartClient()
{
    YOLO_CLIENT->setPort(m_listenPort);
    YOLO_CLIENT->connectServer();
}

void YoloServerManager::onReadyStandarError()
{
    QProcess *p = static_cast<QProcess *>(sender());
    if (p == nullptr) {
        return;
    }

    qDebug() << QString("[ai_yolo] StandarError:%1").arg(p->readAllStandardError().data());
}

void YoloServerManager::onFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    qDebug() << QString("[ai_yolo] exitCode:%1, existStatus:%2").arg(exitCode).arg(exitStatus);

    QTimer::singleShot(50, this, SLOT(start()));
}

void YoloServerManager::onErrorOccurred(QProcess::ProcessError error)
{
    QString msg;
    switch (error)
    {
    case QProcess::FailedToStart:
        msg = QStringLiteral("The process failed to start");
        break;
    case QProcess::Crashed:
        msg = QStringLiteral("The process crashed some time after starting successfully");
        break;
    case QProcess::Timedout:
        msg = QStringLiteral("FailedToStart");
        break;
    case QProcess::ReadError:
        msg = QStringLiteral("An error occurred when attempting to read from the process");
        break;
    case QProcess::WriteError:
        msg = QStringLiteral("An error occurred when attempting to write to the process");
        break;
    default:
        msg = QStringLiteral("An unknown error occurred");
        break;
    }

    qDebug() << QString("[ai_yolo]Error>>%1--%2").arg(error).arg(msg.toUtf8().data());
}

void YoloServerManager::onStateChanged(QProcess::ProcessState newState)
{
     qDebug() << QString("[ai_yolo] state:%1").arg(newState);
}

