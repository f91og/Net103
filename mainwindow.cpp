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
    //SendAsdu21ForNeiBuDingZhi();//貌似有问题
    //    GetDeviceDingZhi();
    GetLuBo();
    //    pthread_t threadidtmp;
    //    pthread_create(&threadidtmp,NULL,GetLuBo,NULL);
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
    pthread_t threadidtmp;
    pthread_create(&threadidtmp,NULL,UDPThread,NULL);
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
            if(a.m_COT==0x02&&a.m_ASDUData.at(1)==0x04)//如果循环上送的是报告，则需特殊处理
            {
                QFile file0("E:\\Net103\\192.168.0.171-01-循环上报-动作报告.txt");
                QString datatype="功能类型和信息序号";
                file0.open(QIODevice::WriteOnly | QIODevice::Text);
                QTextStream in(&file0);
                in.setCodec("UTF-8");
                CAsdu10Link a10link(a);
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
            QSettings *waveFileList=new QSettings("WaveFile/192.168.0.171_CPU1/FileList.ini",QSettings::IniFormat);
            waveFileList->beginGroup("List");
//            CAsdu201 a201(Data); 这里不能再用Data了，以上面的SaveAsdu在最后将Data清空了
            CAsdu201 a201(a);
            if(a201.listNum<=0) return;//有录波文件才召
            for(int i=0;i<a201.listNum;i++)
            {
                int num=a201.m_DataSets.at(i).lubo_num;
                waveFileList->setValue(QString(i+1),QString(num));
            }
            waveFileList->endGroup();
            delete waveFileList;

            QList<CAsdu200> a200_list;
            for(int i=0;i<a201.listNum;i++)
            {

                CAsdu200 a200;
                a200.file_name=a201.m_DataSets.at(i).file_name;
                QByteArray sData;
                a200.BuildArray(sData);
                socket->write(sData);
                socket->flush();
                while(true)
                {
                    qDebug()<<a200.file_name;
                    QByteArray data=socket->readAll();
                    if(data.size()<=0) continue;
                    CAsdu a;
                    a.m_ASDUData.resize(0);
                    a.SaveAsdu(data);
                    CAsdu200 a200_rec(a);
                    if(a200.followTag==0)
                    {
                        QFile file("E:\\Net103\\WaveFile\\192.168.0.171_CPU1\\"+QString::fromLocal8Bit(a200.file_name)+".datagram");
                        file.open(QIODevice::WriteOnly | QIODevice::Text);
                        QTextStream in(&file);
                        for(CAsdu200 a200_i:a200_list)
                        {
                            for(int i=0;i<a200_i.all_packet.size();i++)
                            {
                                in<<a200_i.all_packet.mid(i,i).toHex();
                                if(i!=0&&((i+1)%16==0)) in<<"\n";
                            }
                        }
                        file.close();
                        a200_list.clear();
                        break;
                    }else{
                        a200_list.append(a200_rec);
                    }
                }
            }
            break;
        }
        case 0xc8:  // 收到的是asdu200，将asud200存成datagram数据文件,这里可能会收到不同子站的asdu200
        {   //首先要判断ip和应用服务单元公共地址，确定写入哪个文件夹
            CAsdu200 a200(a);
//            QString str=QString::fromLocal8Bit(name);
            ushort i=0;
            QFile file("E:\\Net103\\WaveFile\\192.168.0.171_CPU1\\录波数据包"+(i++)+".datagram");
            file.open(QIODevice::WriteOnly | QIODevice::Text);
            QTextStream in(&file);
            for(int i=0;i<a200.all_packet.size();i++)
            {
                in<<a200.all_packet.mid(i,i).toHex();
                if(i!=0&&((i+1)%16==0)) in<<"\n";
            }
            in<<"\n";
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
            //qDebug()<<data.gin.GROUP<<"--"<<data.gin.ENTRY<<"--"<<data.kod<<"--"<<data.gid.toHex()<<"\n";
            //遥测的数据类型是带品质描述的被测值，需要特殊处理
            int entry=data.gin.ENTRY;
            QString MEA=QString(data.gid);
            in<<group<<",Entry："<<entry<<MEA<<"\n";
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
                //上面的时间信息是二进制CP32Timea形式的，需还原
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
    }else if(a10.m_COT==0x2a)//通用分类读取命令的有效数据响应
    {
        DataSet pData=a10.m_DataSets.at(0);
        // 读当前运行定值的区号,发Asdu10选择要读取的当前定值的区号
        if(a10.m_RII==0x06||a10.m_RII==0x08)
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
            if(a10.m_RII==0x08){
                sData[6]=0x09;//RII=9标记读装置定值的所有条目asdu21
                sData21[8]=0x03;//组号--定值
                sData21[9]=0x00;//所有条目
            }else
            {
                sData21[6]=0x0a;//RII=10标记读压板所有条目的asdu21
                sData21[8]=0x0e;//组号--压板
                sData21[9]=0x00;//所有条目
            }

            sData21[10]=0x01;
            socket->write(sData21);
            socket->flush();
        }
        else if(a10.m_RII==0x09)//装置定值
        {
            QFile file("E:\\Net103\\192.168.0.171-01-装置定值.txt");
            file.open(QIODevice::WriteOnly | QIODevice::Text);
            QTextStream in(&file);
            for(int i=0;i<a10.m_NGD.byte;i++)
            {
                DataSet d=a10.m_DataSets.at(i);
                in<<"DingZhi--Entry:"<<d.gin.ENTRY<<",Value:"<<d.gid.toHex()<<"\n";
            }
            file.close();
        }
        else if(a10.m_RII==0x07)//内部定值
        {
            QFile file("E:\\Net103\\192.168.0.171-01-装置内部定值.txt");
            file.open(QIODevice::WriteOnly | QIODevice::Text);
            QTextStream in(&file);;
            for(int i=0;i<a10.m_NGD.byte;i++)
            {
                DataSet d=a10.m_DataSets.at(i);
                qDebug()<<"NeiBuDingZhiTest-->";
                qDebug()<<d.gin.GROUP<<"--"<<d.gin.ENTRY<<"--"<<d.gid.toHex();
                in<<"NeiBuDingZhi,"<<"entry:"<<d.gin.ENTRY<<",value:"<<d.gid.toInt()<<"\n";
            }
            file.close();
        }
        else if(a10.m_RII==0x0a)//压板
        {
            QFile file("E:\\Net103\\192.168.0.171-01-软压板.txt");
            file.open(QIODevice::WriteOnly | QIODevice::Text);
            QTextStream in(&file);
            for(int i=0;i<a10.m_NGD.byte;i++)
            {
                DataSet d=a10.m_DataSets.at(i);
                qDebug()<<"YaBan Test---->";
                qDebug()<<d.gin.GROUP<<"--"<<d.gin.ENTRY<<"--"<<d.gid.toHex();
                in<<"YaBan,"<<"entry:"<<d.gin.ENTRY<<",value:"<<d.gid.toInt()<<"\n";
            }
            file.close();
        }
        else if(a10.m_RII<3)//模拟信道
        {
            QSettings *moni=new QSettings("Device_Info/WaveChannel/192.168.0.171_CPU1.ini",QSettings::IniFormat);
            moni->setValue("/Device/ID",QString(a10.m_Addr));
            moni->setValue("/Device/NAME","10kV线路保护");
            moni->setValue("/Device/ANALOG_CHANNEL",(int)a10.m_NGD.byte);
            moni->setValue("/Device/CREATE_TIME","????");//要从数据库中查？
            QTextCodec *tc=QTextCodec::codecForName("GBK");
            for(int i=0;i<a10.m_DataSets.size();i++)
            {
                QString key=QString("ANALOG_CHANNEL/")+a10.m_DataSets.at(i).gin.ENTRY;
                if(a10.m_RII==0)//如果返回的是通道描述
                {
                    QString description=tc->toUnicode(a10.m_DataSets.at(i).gid)+",1";
                    moni->setValue(key,description);
                }
                else if(a10.m_RII==1)//量纲
                {
                    QString formalValue=moni->value(key).toString();
                    QString newValue=formalValue+","+tc->toUnicode(a10.m_DataSets.at(i).gid);
                    moni->setValue(key,newValue);
                }
                else if(a10.m_RII==2)//实际值，计算系数
                {
                    QString formalValue=moni->value(key).toString();
                    QString newValue=formalValue+","+a10.m_DataSets.at(i).gid.toFloat();
                    moni->setValue(key,newValue);
                }
            }
            delete moni;
        }
        else if(a10.m_RII==3||a10.m_RII==4)//数字信道,3表示返回的是描述，4表示返回的是实际值
        {
            QSettings *shuzi=new QSettings("Device_Info/WaveChannel/192.168.0.171_CPU1.ini",QSettings::IniFormat);
            shuzi->setValue("/Device/ID",QString(a.m_Addr));
            shuzi->setValue("/Device/NAME","10kV线路保护");
            shuzi->setValue("/Device/DIGIT_CHANNEL",(int)a10.m_NGD.byte);
            qDebug()<<a10.m_DataSets.size();//调查结果出来了，是接受封装的asdu10不对
            QTextCodec *tc=QTextCodec::codecForName("GBK");
            for(int i=0;i<a10.m_DataSets.size();i++)
            {
                QString key=QString("DIGIT_CHANNEL/")+a10.m_DataSets.at(i).gin.ENTRY;
                if(a10.m_RII==3)
                {
                    QString description=tc->toUnicode(a10.m_DataSets.at(i).gid)+",2";
                    shuzi->setValue(key,description);
                }
                else if(a10.m_RII==4)
                {
                    QString formalValue=shuzi->value(key).toString();
                    QString newValue=formalValue+","+a10.m_DataSets.at(i).gid.toFloat();
                    shuzi->setValue(key,newValue);
                }
            }
            delete shuzi;
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
    //先发ASDU21读取运行值区号
    CAsdu21 a21;
    a21.m_Addr=0x01;//从别处获得
    a21.m_INF=0xf4;//读单个条目的值或属性
    a21.m_NOG=0x01;
    a21.m_RII=0x06;//06标记读压板时发送的读运行值区号asdu
    DataSet *pDataSet=new DataSet;
    pDataSet->gin.GROUP=0x00;
    pDataSet->gin.ENTRY=0x03;
    pDataSet->kod=0x01;//kod=1表示要读的是条目的实际值，而不是条目的描述
    a21.m_DataSets.append(*pDataSet);
    QByteArray data;
    a21.BuildArray(data);
    socket->write(data);
    socket->flush();
}

void MainWindow::SendAsdu21ForNeiBuDingZhi()
{
    CAsdu21 a21;
    a21.m_Addr=0x01;
    a21.m_RII=0x07;
    a21.m_NOG=0x01;
    a21.m_INF=0xf1;
    DataSet *pDataSet=new DataSet;
    pDataSet->gin.GROUP=0x02;
    pDataSet->gin.ENTRY=0x00;
    pDataSet->kod=0x01;//kod=1表示要读的是条目的实际值，而不是条目的描述
    a21.m_DataSets.append(*pDataSet);
    QByteArray sData;
    a21.BuildArray(sData);
    socket->write(sData);
    socket->flush();
}

void MainWindow::GetDeviceDingZhi() //读当前定值
{
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
    data[6]=0x08;//08标记读压板时发送的读运行值区号asdu
    data[7]=0x01;
    data[8]=0x00;//组号
    data[9]=0x03;//条目号
    data[10]=0x01;//kod=1表示要读的是条目的实际值，而不是条目的描述
    socket->write(data);
    socket->flush();
}

void MainWindow::GetLuBo()
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

    //设备信息是要到数据库定时插再写入ini文件中的
    /*设备通道信息分为数字通道，和模拟通道。模拟通道除了需要召唤数据外，还需要召唤通道描述，量纲，计算系数(实际值)。
          数字通道则只需要召唤描述和数据。模拟通道的组号是0x1D，数字通道的组号是0x20。要分开召*/
    QByteArray sData;
//    CAsdu21 a21;
//    DataSet *pDataSet=NULL;
//    //模拟通道--描述
//    a21.m_INF=0xf1;//读一个组的全部条目
//    a21.m_Addr=0x01;//
//    a21.m_NOG=0x01;
//    for(int i=0;i<5;i++)
//    {
//        a21.m_RII=i;
//        pDataSet=new DataSet;
//        //召唤模拟通道所有条目的描述，量纲，计算系数(实际值)
//        if(i<3)
//        {
//            pDataSet->gin.GROUP=0x1d;
//            pDataSet->gin.ENTRY=0x00;
//            if(i==0) pDataSet->kod=10;//通道描述
//            if(i==1) pDataSet->kod=9;//量纲（单位）
//            if(i==2) pDataSet->kod=1;//实际值
//        }else
//        {
//            pDataSet->gin.GROUP=0x20;
//            pDataSet->gin.ENTRY=0x00;
//            if(i==3) pDataSet->kod=10;
//            if(i==4) pDataSet->kod=1;
//        }
//        a21.m_DataSets.clear();
//        a21.m_DataSets.append(*pDataSet);
//        pDataSet=NULL;
//        a21.BuildArray(sData);
//        socket->write(sData);
//        socket->flush();
//        //sleep(5);
//    }

    //故障录波文件
    CAsdu201 a201;
    sData.resize(0);
    a201.BuildArray(sData);
    socket->write(sData);
    socket->flush();
    //sleep(60);
}
