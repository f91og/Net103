#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpServer>
#include <QTcpSocket>
#include <QMap>
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

signals:
    void reciveAllLuBoPakcet(const QByteArray & file_name) const;

private:
    void ExplainASDU(QByteArray& Data);
    void ProcessAsdu10(CAsdu& a);

    void SendAsdu07();
    void SendAsdu21ForYaBan();
    void SendAsdu21ForNeiBuDingZhi();
    void GetDeviceDingZhi();
    void GetDingZhi();
    void GetYaBan();
    void GetLuBo(const QByteArray & file_name);//这个和不加const和引用的传参有什么区别？
    void BeforeGetLuBo();
    void GetBaoHu();
    void SendAsdu21(const BYTE &m_addr, const BYTE &m_RII, const BYTE &m_INF,const BYTE &NGD, DataSet *pDataSet);

private slots:

    void server_New_Connect();
    void socket_Read_Data();
    void socket_Disconnected();
    void on_start_button_clicked();
    void onTimerOut();

private:
    Ui::MainWindow *ui;
    QTcpServer* server;
    QTcpSocket* socket;
    QTimer *timer;

    QList<QByteArray> luBo_list;
    QList<CAsdu200> a200_list;
    QList<QTcpSocket*> socket_list;
};

#endif // MAINWINDOW_H
