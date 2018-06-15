#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "mainstation.h"
#include "asdu.h"
#include <time.h>
#include <pthread.h>
#include <QFile>
#include <QTextCodec>
#include <QDir>
#include <unistd.h>
#include <QSettings>

bool isGetdingzhi;
QTcpServer* server;
QTcpSocket* socket;

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
//    SendAsdu07();
//    SendAsdu21ForYaBan();
//    SendAsdu21ForNeiBuDingZhi();
//    GetDeviceDingZhi();
    pthread_t threadidtmp;
    pthread_create(&threadidtmp,NULL,UDPThread,NULL);
    pthread_create(&threadidtmp,NULL,GetLuBo,NULL);
    QObject::connect(socket, &QTcpSocket::readyRead, this, &MainWindow::socket_Read_Data);
    QObject::connect(socket, &QTcpSocket::disconnected, this, &MainWindow::socket_Disconnected);
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
            if((BYTE)Data[2]==0x02&&(BYTE)Data[8]==0x04)//如果循环上送的是报告，则需特殊处理
            {
                QFile file0("E:\\Net103\\192.168.0.171-01-循环上报-动作报告.txt");
                QString datatype="功能类型和信息序号";
                file0.open(QIODevice::WriteOnly | QIODevice::Text);
                QTextStream in(&file0);
                in.setCodec("UTF-8");
                CAsdu10Link a10link(Data);
                in<<"动作报告--"<<"条目号："<<a10link.gin_h.ENTRY<<"--"
                 <<"共有"<<a10link.reportArgNum<<"个动作参数"
                 <<"--通用分类标识数据："<<a10link.d1.gid<<"\n";
                for(int i=0;i<a10link.reportArgNum;i++)
                {
                    in<<"描述(8位ASCLL码)："<<a10link.m_DataSet.at(i).gid;
                    in<<"量纲(8位ASCLL码)："<<a10link.m_DataSet.at(i+1).gid;
                    in<<"实际值(<1><2><3><4><5><6><7>)："<<a10link.m_DataSet.at(i).gid;
                    i=i+3;
                }
                file0.close();
            }else
            {
               ProcessAsdu10(a);
            }
            break;
        case 0xc9:  // 收到的是asdu201
        {
            QList<CAsdu200> Asdu200List;
            //Qt中使用QSetting类读写ini文件,https://blog.csdn.net/qiurisuixiang/article/details/7760828
            QSettings *waveFileList=new QSettings("WaveFile/192.168.0.171_CPU1/FileList.ini",QSettings::IniFormat);
            waveFileList->beginGroup("List");
            CAsdu201 a201(Data);
            if(a201.listNum<=0) return;//有录波文件才召
            for(int i=0;i<a201.listNum;i++)
            {
                int num=a201.m_DataSets.at(i).lubo_num;
                waveFileList->setValue(QString(i+1),QString(num));
                CAsdu200 a200;
                a200.file_name=a201.m_DataSets.at(i).file_name;
                QByteArray sData;
                a200.BuildArray(sData);
                socket->write(sData);
                socket->flush();
            }
            waveFileList->endGroup();
            delete waveFileList;
            break;
        }
        case 0xc8:  // 收到的是asdu200，将asud200存成datagram数据文件,这里可能会收到不同子站的asdu200
        {   //首先要判断ip和应用服务单元公共地址，确定写入哪个文件夹
            QByteArray name=Data.mid(13,31);//取文件名称
            QTextCodec *codec=QTextCodec::codecForName("GBK");//解决乱码问题
            QString str=codec->toUnicode(name);
            QFile file("E:\\Net103\\WaveFile\\192.168.0.171_CPU1\\"+str+".datagram");
            file.open(QIODevice::WriteOnly);
            QTextStream in(&file);
            in<<Data;
            file.close();
            break;
        }
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
        in.setCodec(QTextCodec::codecForName("GB2312"));
        QString group;
        for(int i=0;i!=a10.m_NGD.byte;i++)
        {
            DataSet data=a10.m_DataSets.at(i);
            switch (data.gin.GROUP) {
            case 0x08:case 0x09:
                group="遥信";
                break;
            case 0x07:
                group="遥测";
                break;
            default:
                break;
            }
            qDebug()<<data.gin.GROUP<<"--"<<data.gin.ENTRY<<"--"<<data.kod<<"--"<<data.gid[0]<<"\n";
            in<<group<<",Entry："<<data.gin.ENTRY<<",Value："<<data.gid[0]<<"\n";
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
            if(data.gin.GROUP==0x05)//告警
            {
                in<<"组号：05H(告警)--条目号："<<data.gin.ENTRY
                 <<"实际值(1=开，2=合)："<<gid[0]<<"--时间:"<<
                 gid[4]<<"h"<<gid[3]<<"min"<<gid[1]<<gid[2]<<"ms"<<"\n";
                //上面的时间信息是二进制的，需要还原
            }else if(data.gin.GROUP==0x08)//遥信变位时的带时标的遥信SOE上传
            {
                in<<"组号：08H(告警)--条目号："<<data.gin.ENTRY<<"实际值(1=开，2=合)"<<gid[0];
                DataSet data=a10.m_DataSets.at(i+1);
                in<<"----时标信息--条目号："<<data.gin.ENTRY<<"，实际值：(1=开，2=合)"<<data.gid[0]<<
                    ",时间："<<gid[4]<<"h"<<gid[3]<<"min"<<gid[1]<<gid[2]<<"ms"<<"\n";
                i=i+1;
            }
        }
        file2.close();
    }else if(a10.m_COT==0x2a)//通用分类度命令的有效数据响应
    {
        DataSet pData=a10.m_DataSets.at(0);//将Asdu10查询到的运行定值区号(gid数据中)赋给pData
        // 读当前运行定值的区号,发Asdu10选择要读取的当前定值的区号
        if(a10.m_INF==0xf4&&pData.gin.GROUP==0x00&&pData.gin.ENTRY==0x03)
        {
            //发Asdu10选择要写入的定值的区号,这个区号是放在gid里的一个字节，通过指定这个区号可以实现读指定区号的定值
            CAsdu10 a10_send;
            a10_send.m_Addr=0x01;//这个addr需从别处查来的
            a10_send.m_INF=0xf9;
            a10_send.m_RII=0x14;
            a10_send.m_NGD.byte=0x01;
            //关于C++中结构体如何实例和赋值？
            DataSet* data=NULL;
            data=new DataSet;
            data->gin.GROUP=0x00;
            data->gin.ENTRY=0x02;
            data->gdd.gdd.DataType=0x03;
            data->gdd.gdd.DataSize=0x01;
            data->gdd.gdd.Number=0x01;
            data->gid=pData.gid;//将pData中的区号给data，封装在asdu10写命令中
            a10.m_DataSets.clear();
            a10.m_DataSets.append(*data);
            QByteArray sData;
            a10.BuildArray(sData);
            socket->write(sData);
            socket->flush();

            //发Asdu21读03H组（定值）的全部条目
            QByteArray sData21;
            sData21.resize(11);
            sData21[0]=0x15;
            sData21[1]=0x81;
            sData21[2]=0x2a;
            sData21[3]=0x01;
            sData21[4]=0xfe;
            sData21[5]=0xf1;
            sData21[6]=0x15;
            sData21[7]=0x01;
            if(isGetdingzhi){
                sData21[8]=0x03;//组号--定值
                sData21[9]=0x00;//条目号
            }else
            {
                sData21[8]=0x0e;//组号--压板
                sData21[9]=0x00;//条目号
            }

            sData21[10]=0x01;
            socket->write(sData21);
            socket->flush();
        }
        else if(a10.m_INF==0xf1&&pData.gin.GROUP==0x03)//装置定值
        {
            QFile file("E:\\Net103\\192.168.0.171-01-装置定值.txt");
            file.open(QIODevice::WriteOnly | QIODevice::Text);
            QTextStream in(&file);
            in.setCodec("UTF-8");
            for(int i=0;i<a10.m_NGD.byte;i++)
            {
                DataSet d=a10.m_DataSets.at(i);
                in<<"条目:"<<d.gin.ENTRY<<"--值："<<d.gid<<"\n";
            }
            file.close();
        }
        else if(a10.m_INF==0xf1&&pData.gin.GROUP==0x02)//内部定值
        {
            QFile file("E:\\Net103\\192.168.0.171-01-装置内部定值.txt");
            file.open(QIODevice::WriteOnly | QIODevice::Text);
            QTextStream in(&file);
            in.setCodec("UTF-8");;
            for(int i=0;i<a10.m_NGD.byte;i++)
            {
                DataSet d=a10.m_DataSets.at(i);
                in<<"条目:"<<d.gin.ENTRY<<"--值："<<d.gid<<"\n";
            }
            file.close();
        }
        else if(a10.m_INF==0xf1&&pData.gin.GROUP==0x0e)//压板
        {
            QFile file("E:\\Net103\\192.168.0.171-01-软压板.txt");
            file.open(QIODevice::WriteOnly | QIODevice::Text);
            QTextStream in(&file);
            in.setCodec("UTF-8");;
            for(int i=0;i<a10.m_NGD.byte;i++)
            {
                DataSet d=a10.m_DataSets.at(i);
                in<<"条目:"<<d.gin.ENTRY<<"--值："<<d.gid<<"\n";
            }
            file.close();
        }
        else if(a10.m_INF==0xf1&&pData.gin.GROUP==0x1D)//模拟信道
        {
            QSettings moni("Device_Info/WaveChannel/192.168.0.171_CPU1.ini",QSettings::IniFormat);
            moni.setValue("/Device/ID",QString(a.m_Addr));
            moni.setValue("/Device/NAME","10kV线路保护");
            moni.setValue("/Device/ANALOG_CHANNEL",(int)a10.m_NGD.byte);
            moni.setValue("/Device/CREATE_TIME","????");
            for(int i=0;i<a10.m_NGD.byte;i++)
            {
                QString key=QString("WAVE_CHANNEL/")+QString("通道ID");//具体的id值是数据？
                QString value="???";
                moni.setValue(key,value);
            }
        }
        else if(a10.m_INF==0xf1&&pData.gin.GROUP==0x20)//数字信道
        {
            QSettings shuzi("Device_Info/WaveChannel/192.168.0.171_CPU1.ini",QSettings::IniFormat);
            shuzi.setValue("/Device/ID",QString(a.m_Addr));
            shuzi.setValue("/Device/NAME","10kV线路保护");
            shuzi.setValue("/Device/DIGIT_CHANNEL",(int)a10.m_NGD.byte);
            shuzi.setValue("/Device/CREATE_TIME","????");
            for(int i=0;i<a10.m_NGD.byte;i++)
            {
                QString key=QString("WAVE_CHANNEL/")+QString("通道ID");
                QString value="???";
                shuzi.setValue(key,value);
            }
        }
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

void MainWindow::SendAsdu21ForYaBan()
{
    isGetdingzhi=false;
    //先发ASDU21读取运行值区号
    QByteArray data;
    data.resize(11);
    data.resize(11);
    data[0]=0x15;
    data[1]=0x81;
    data[2]=0x2a;
    data[3]=0x01;
    data[4]=0xfe;
    data[5]=0xf4;
    data[6]=0x13;
    data[7]=0x01;
    data[8]=0x00;//组号
    data[9]=0x03;//条目号
    data[10]=0x01;//kod=1表示要读的是条目的实际值，而不是条目的描述
    socket->write(data);
    socket->flush();
}

void MainWindow::SendAsdu21ForNeiBuDingZhi()
{
    QByteArray data;
    data.resize(11);
    data[0]=0x15;
    data[1]=0x81;
    data[2]=0x2a;
    data[3]=0x01;
    data[4]=0xfe;
    data[5]=0xf1;
    data[6]=0x00;
    data[7]=0x01;
    data[8]=0x02;//组号
    data[9]=0x00;//条目号
    data[10]=0x01;
    socket->write(data);
    socket->flush();
}

void MainWindow::GetDeviceDingZhi() //读当前定值
{
    isGetdingzhi=true;
    //先发ASDU21读取运行值区号
    QByteArray data;
    data.resize(11);
    data.resize(11);
    data[0]=0x15;
    data[1]=0x81;
    data[2]=0x2a;
    data[3]=0x01;
    data[4]=0xfe;
    data[5]=0xf4;
    data[6]=0x13;
    data[7]=0x01;
    data[8]=0x00;//组号
    data[9]=0x03;//条目号
    data[10]=0x01;//kod=1表示要读的是条目的实际值，而不是条目的描述
    socket->write(data);
    socket->flush();
}

void *GetLuBo(void *lp)
{
    //生成故障录波文件目录
    QDir dir;
    if(!dir.exists("WaveFile"))
    {
        dir.mkdir("WaveFile");
    }
    //生成设备信息转存文件目录
    if(!dir.exists("Device_Info/Device"))
    {
        dir.mkdir("Device_Info/Device");
    }
    if(!dir.exists("Device_Info/WaveChannel"))
    {
        dir.mkdir("Device_Info/WaveChannel");
    }

    //向数据库查询现有装置的ip和cpu号，写文件目录和文件，我这里先固定写一个装置的情形
    dir.mkdir("WaveFile/192.168.0.171_CPU1");
    while(true)
    {
        //设备信息是要到数据库定时插再写入ini文件中的
        /*设备通道信息分为数字通道，和模拟通道。模拟通道除了需要召唤数据外，还需要召唤通道描述，量纲，计算系数。
          数字通道则只需要召唤描述和数据。模拟通道的组号是0x1D，数字通道的组号是0x20*/
        //模拟通道
        CAsdu21 a21ForMoni;
        a21ForMoni.m_NOG=4;
        DataSet *pDataSet1=new DataSet;
        pDataSet1->gin.GROUP=0x1d;
        pDataSet1->gin.ENTRY=0x00;
        pDataSet1->kod=10;//通道描述
        a21ForMoni.m_DataSets.append(*pDataSet1);

        DataSet *pDataSet2=new DataSet;
        pDataSet2->gin.GROUP=0x1d;
        pDataSet2->gin.ENTRY=0x00;
        pDataSet2->kod=9;//量纲（单位？）
        a21ForMoni.m_DataSets.append(*pDataSet2);

        DataSet *pDataSet3=new DataSet;
        pDataSet3->gin.GROUP=0x1d;
        pDataSet3->gin.ENTRY=0x00;
        pDataSet3->kod=6;//因子是计算系数？
        a21ForMoni.m_DataSets.append(*pDataSet3);

        DataSet *pDataSet4=new DataSet;
        pDataSet4->gin.GROUP=0x1d;
        pDataSet4->gin.ENTRY=0x00;
        pDataSet4->kod=1;//实际值(数据)？
        a21ForMoni.m_DataSets.append(*pDataSet4);
        QByteArray sDataForMoni;
        a21ForMoni.BuildArray(sDataForMoni);
        socket->write(sDataForMoni);
        socket->flush();

        //数字通道
        CAsdu21 a21ForShuZi;
        a21ForShuZi.m_NOG=2;
        DataSet *pDataSet5=new DataSet;
        pDataSet5->gin.GROUP=0x20;
        pDataSet5->gin.ENTRY=0x00;
        pDataSet5->kod=10;//通道描述
        a21ForShuZi.m_DataSets.append(*pDataSet5);

        DataSet *pDataSet6=new DataSet;
        pDataSet6->gin.GROUP=0x20;
        pDataSet6->gin.ENTRY=0x00;
        pDataSet6->kod=1;
        a21ForShuZi.m_DataSets.append(*pDataSet6);
        QByteArray sDataForShuZi;
        a21ForShuZi.BuildArray(sDataForShuZi);
        socket->write(sDataForShuZi);
        socket->flush();

        //故障录波文件
        CAsdu201 a201;
        QByteArray sData;
        a201.BuildArray(sData);
        socket->write(sData);
        socket->flush();
        sleep(60);
    }
}
