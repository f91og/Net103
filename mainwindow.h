#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpServer>
#include <QTcpSocket>
#include "asdu.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    void ExplainASDU(QByteArray& Data);
    void ProcessAsdu10(CAsdu& a);

    void SendAsdu07();
    void SendAsdu21ForYaBan();
    void SendAsdu21ForNeiBuDingZhi();
    void GetDeviceDingZhi();
    void GetDingZhi();
    void GetYaBan();
    void GetLuBo();

private slots:

    void server_New_Connect();

    void socket_Read_Data();

    void socket_Disconnected();

    void on_start_button_clicked();

private:
    Ui::MainWindow *ui;
};

//void *GetLuBo(void *lp);
#endif // MAINWINDOW_H
