#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpServer>
#include <QTcpSocket>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:

    void server_New_Connect();

    void socket_Read_Data();

    void socket_Disconnected();

    void on_start_button_clicked();

private:
    Ui::MainWindow *ui;
    QTcpServer* server;
    QTcpSocket* socket;
};

#endif // MAINWINDOW_H
