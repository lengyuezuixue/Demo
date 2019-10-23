#ifndef MAIN_WIDGET_H_0121
#define MAIN_WIDGET_H_0121

#include <QtWidgets/QWidget>

namespace Ui {
	class MainWidget;
}

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

private:
    void testImage();

private:
	Ui::MainWidget* ui;

    QPixmap m_pix;

};

#endif


