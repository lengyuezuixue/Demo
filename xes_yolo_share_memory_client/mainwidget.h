#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include <QWidget>
#include "SharedMemory.h"

namespace Ui {
class MainWidget;
}

class MainWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MainWidget(QWidget *parent = nullptr);
    ~MainWidget();

private slots:
    void on_pushButton_clicked();

private:
    Ui::MainWidget *ui;

    sm::SystemSemaphore *m_sysSemaphore;
    sm::SharedMemory m_sharedMemory;
};

#endif // MAINWIDGET_H
