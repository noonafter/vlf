//
// Created by noisemonitor on 2024/12/16.
//

#include "VLFUdpReceiver.h"

VLFUdpReceiver::VLFUdpReceiver() {

    udp_socket = new QUdpSocket(this);


}

void VLFUdpReceiver::slot_receiver_readyRead() {

    // 包接收
    while (udp_socket->hasPendingDatagrams()){
        QNetworkDatagram datagram = udp_socket->receiveDatagram();

        // 包处理
        VLFAbstractReceiver::process_package(datagram.data());

    }

}

VLFUdpReceiver::~VLFUdpReceiver() {

}

int VLFUdpReceiver::startReceiving() {

    // 1MB udp buffer
    udp_socket->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption,  1<<20);
    // 注册包接收函数
    connect(udp_socket, &QUdpSocket::readyRead, this, &VLFUdpReceiver::slot_receiver_readyRead);
    // 监听对应数据端口
    if(udp_socket->bind(QHostAddress::LocalHost, 55556)){
        qDebug() << "Bind success";
    }else {
        qDebug() << "Udp socket bind fail: " << udp_socket->errorString();
    };
    return 0;
}
