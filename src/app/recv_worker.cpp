//
// Created by noisemonitor on 2024/11/28.
//

#include <QNetworkDatagram>
#include "recv_worker.h"


recv_worker::recv_worker(QObject *parent) : QObject(parent){

    udp_recv = new QUdpSocket(this);
    if(udp_recv->bind(QHostAddress::LocalHost, 55555)){
        qDebug() << "Bind success";
    }else {
        qDebug() << "Udp socket bind fail: " << udp_recv->errorString();
    };
    // 1MB udp buffer
    udp_recv->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption,  1<<20);
    connect(udp_recv, &QUdpSocket::readyRead, this, &recv_worker::slot_udp_recv_readyRead);

    file = new QFile(R"(E:\project\vlf\src\udp_modem\sig_sum_int)");
    if(!file->open(QIODevice::WriteOnly | QIODevice::Append)){
        qWarning() << "failed to open file" << file->errorString();
        return;
    }
    out = new QDataStream(file);

}

recv_worker::~recv_worker() {

    if(file)
        file->close();
    delete file;
    delete out;
}

void recv_worker::slot_udp_recv_readyRead() {
//    qDebug() << "slot udp recv";


    while (udp_recv->hasPendingDatagrams()){
        QNetworkDatagram datagram = udp_recv->receiveDatagram();

        // 处理datagram
        QByteArray aba = datagram.data();
        out->writeRawData(aba,aba.size());
        count_rev++;
    }

    if(!(count_rev % 100)){
        qDebug() << "count_rev is" << count_rev;
//        qDebug() << "recv buff is " << udp_recv->socketOption(QAbstractSocket::ReceiveBufferSizeSocketOption);
    }





}
