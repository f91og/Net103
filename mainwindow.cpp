#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "mainstation.h"
#include <time.h>
#include <pthread.h>
#include <QFile>
#include "asdu.h"

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
    SendAsdu07();//这里直接这么用可以么？
    SendAsdu21ForYaoCe();
    SendAsdu21ForDingZhi();
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
    ExplainASDU(buffer);
}

void MainWindow::socket_Disconnected()
{
    qDebug() << "Disconnected......";
}

void MainWindow::on_start_button_clicked()
{
    ui->textEdit_Recv->setText("开始连接子站......");
    pthread_t threadidtmp;
    pthread_create(&threadidtmp,NULL,UDPThread,NULL);
    server->listen(QHostAddress::Any, 1048);
}

void MainWindow::ExplainASDU(QByteArray &Data)
{
    CAsdu a;
    do
    {
        a.m_ASDUData.resize(0);
        a.SaveAsdu(Data); //这步结束后只组装了一般的Asdu，并不知道具体类型，但是asdu中携带的数据已经封装到m_AsduData中了
        if(a.m_iResult==0) return;
        switch (a.m_TYP) {
        case 0x0a:
            ProcessAsdu10(a);
            break;
        case 201:
            // process asdu201
            break;
        case 200:
            // process asdu200
            break;
        default:
            break;
        }
    }while(a.m_ASDUData.size()>0);
}

void MainWindow::ProcessAsdu10(CAsdu &a)
{
    CAsdu10 a10(a); //将原来a中的数据赋给a10
    a10.ExplainAsdu(); //正式开始解析这个asdu10包
    if(a10.m_COT==0x02){ //循环上报的数据
        QString addr=QString(a10.m_Addr);
        int i_addr=a10.m_Addr;  // 这里uchar转int直接赋值就可以了
        QFile file1("E:\\Net103\\192.168.0.171-01-循环上报.txt");
        file1.open(QIODevice::WriteOnly | QIODevice::Text);
        QTextStream in(&file1);
        in.setCodec("UTF-8");
        QString str="组号  条目  描述类别  数据";
        in<<str<<"\n";
        for(int i=0;i!=a10.m_NGD.byte;i++)
        {
            DataSet data=a10.m_DataSets.at(i);
            QString group;
            int entry=data.gin.ENTRY;
            int kod=data.kod;
            QByteArray gid=data.gid;
            switch (data.gin.GROUP) {
            case 0x03:case 0x02:
                group="定值";
                break;
            case 0x08:case 0x09:
                group="遥信";
                break;
            case 0x07:
                group="运动测量";
                break;
            default:
                break;
            }
            //data.gdd.taggdd.DataType; 这里好像是9，双点信息
            in<<group<<"   "<<entry<<"     "<<kod<<"   "<<gid<<"\n";
        }
        file1.close();
    }
    else if(a10.m_COT==0x01)//突发数据
    {
        QFile file2("E:\\Net103\\192.168.0.171-01-突发上传.txt");
        file2.open(QIODevice::WriteOnly | QIODevice::Text);
        QTextStream in(&file2);
        in.setCodec("UTF-8");
        for(int i=0;i!=a10.m_NGD.byte;i++)
        {
            DataSet data=a10.m_DataSets.at(i);
            QByteArray gid=data.gid;
            QString group;
            if(data.gin.GROUP==0x05)//告警
            {
                group="告警";
                QString shuangdian;
                if((BYTE)gid[0]==0x01)
                    shuangdian="开";
                else
                    shuangdian="合";
                in<<group<<"   "<<"条目号："<<data.gin.ENTRY
                 <<"   "<<"双点信息："<<gid[0]<<"   "<<"时间:"<<
                   gid[4]<<"h"<<gid[3]<<"min"<<gid[1]<<gid[2]<<"ms"<<"\n";
                //上面的时间信息是二进制的，需要还原
            }else if(data.gin.GROUP==0x08)//遥信变位时的带时标的要信上传
            {

            }
        }
        file2.close();
    }else if(a10.m_COT==0x2a)
    {
        QFile file3("E:\\Net103\\192.168.0.171-01-突发上传.txt");
        file3.open(QIODevice::WriteOnly | QIODevice::Text);
        QTextStream in(&file3);
        in.setCodec("UTF-8");
        for(int i=0;i!=a10.m_NGD.byte;i++)
        {
            DataSet data=a10.m_DataSets.at(i);
            QByteArray gid=data.gid;
            QString group;
            QString OV;
            QString ER;
            QString RES;
            QString MVAL;
            //这里针对<12>带品质描述的被测值需要精确到bit位上
            if(data.gin.GROUP==0x0E)
            {
                group="遥测";
                // 处理遥测
            }else if(data.gin.GROUP==0x02)
            {
                group="定值";
                //处理定值
            }
        }
        file3.close();
    }
}

void MainWindow::SendAsdu07()
{
    CAsdu07 a07;
    a07.m_Addr=0xff;//这里是广播发送，我感觉应该是到数据库中取才对
    QByteArray sData;
    a07.BuildArray(sData);
    socket->write(sData);
    socket->flush();
}

void MainWindow::SendAsdu21ForYaoCe()
{
    QByteArray data;
    data.resize(11);
    CAsdu21 a21;
    a21.m_Addr=0x01;//后面要改的这个
    a21.m_NOG=1;
    a21.BuildArray(data);
    BYTE group=0x0e;
    BYTE entry=0x00;
    BYTE kod=1;
    data.append(group);
    data.append(entry);
    data.append(kod);
    socket->write(data);
    socket->flush();
}

void MainWindow::SendAsdu21ForDingZhi()
{
    QByteArray data;
    data.resize(11);
    CAsdu21 a21;
    a21.m_Addr=0x01;//后面要改的这个
    a21.m_NOG=1;
    a21.BuildArray(data);
    BYTE group=0x02;
    BYTE entry=0x00;
    BYTE kod=1;
    data.append(group);
    data.append(entry);
    data.append(kod);
    socket->write(data);
    socket->flush();
}

void MainWindow::GetLuBo()
{
    //1.发ASDU201
    //2.子站返回ASDU201，如果有文件列表则发ASDU200
    //3. 子站返回ASDU200
}
