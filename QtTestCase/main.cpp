#include <QCoreApplication>
#include <QDebug>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    // 测试分割字符串的方法
    QString str="192.168.0.171";
    qDebug()<<str.section(".",3,3).toUInt();//结果是171

    return a.exec();
}
