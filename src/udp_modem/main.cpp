#include "udp_modem_widget.h"
#include <QDebug>
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    udp_modem_widget umw;
    int rtn = umw.init();
    if(rtn  < 0)
    {
        qDebug() << "udp modem init error";
        return rtn;
    }
    umw.show();
    return a.exec();
}
