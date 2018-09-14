#include "scada/scadaapi.h"
#include <QtDebug>
#include <QCoreApplication>
#include "pro103app.h"
#include "license/licapp.h"

SCADA_DECL

int ServModuleInit()
{
    LicApp lic;
    if(!lic.CheckLic("p103")){
        qDebug()<<"103模块许可校验失败。";
        return 1;
    }
    if(Pro103App::GetInstance()->Start()){
        qDebug()<<"103模块初始化成功。";
        return 1;
    }
    else{
        qDebug()<<"103模块初始化失败。";
    }
    return 0;
}




