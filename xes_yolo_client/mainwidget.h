#ifndef MAIN_WIDGET_H_0121
#define MAIN_WIDGET_H_0121

#include <QtWidgets/QWidget>
#include <QElapsedTimer>

namespace Ui {
	class MainWidget;
}

class QTimer;
class MainWidget : public QWidget
{
	Q_OBJECT

public:
	MainWidget(QWidget *parent = Q_NULLPTR);


protected:
    void paintEvent(QPaintEvent *ev);

private slots:
    void on_pushButton_clicked();
    void onNewImageAvailabled(const QImage &img);

    void onTimeout();

private:
    void testImage();

private:
	Ui::MainWidget* ui;

    QPixmap m_pix;

    QTimer *m_pTimer;
    bool m_uploading;
    quint64 m_uploadNum;
    QElapsedTimer m_elapsedTimer;

};

#endif


