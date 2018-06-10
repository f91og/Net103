#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "mainstation.h"
#include <time.h>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    server = new QTcpServer();
    connect(server,&QTcpServer::newConnection,this,&MainWindow::server_New_Connect);
}

MainWindow::~MainWindow()
{
    server->close();
    server->deleteLater();
    delete ui;
}

void MainWindow::server_New_Connect()
{
    socket = server->nextPendingConnection();
    ui->textEdit_Recv->setText("客户端已连接......");
    QObject::connect(socket, &QTcpSocket::readyRead, this, &MainWindow::socket_Read_Data);
    QObject::connect(socket, &QTcpSocket::disconnected, this, &MainWindow::socket_Disconnected);
//    uchar data[14];
//    memset(data, 0x00, sizeof(data));
//    data[0]=201;
//    data[1]=0x81;
//    data[2]=0x20;
//    data[3]=0x01;
//    data[4]=0xff;
//    data[5]=0x00;

//    // 获取time_t格式的时间
//    time_t time_start;
//    time_t time_current;
//    int interval=30;

//    time(&time_current);
//    time_start=time_current-interval*24*60*60;

//    memcpy(&data[6], &time_start, 4);
//    memcpy(&data[10], &time_current, 4);
//    socket->write((char*)data,14);
//    socket->flush();
}

void MainWindow::socket_Read_Data()
{
    QByteArray buffer;
    //读取缓冲区数据
    buffer = socket->readAll();
    if(buffer[0]==0x0a){

    }else if(buffer[0]==0x05){

    }else if(buffer[1]==201){

    }
}

void MainWindow::socket_Disconnected()
{
    qDebug() << "Disconnected!";
}

void MainWindow::on_start_button_clicked()
{
    ui->textEdit_Recv->setText("开始连接子站......");
    SendUdpBrocastThread *sendUdpBrocastThread=new SendUdpBrocastThread();
    sendUdpBrocastThread->start();
    qDebug()<<"New thread started...";
    server->listen(QHostAddress::Any, 1048);
}
