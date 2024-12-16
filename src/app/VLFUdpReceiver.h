//
// Created by noisemonitor on 2024/12/16.
//

#ifndef VLF_VLFUDPRECEIVER_H
#define VLF_VLFUDPRECEIVER_H

#include "vlfabstractreceiver.h"
#include <QUdpSocket>
#include <QNetworkDatagram>

class VLFUdpReceiver : public VLFAbstractReceiver {

public:
    explicit VLFUdpReceiver();

    virtual ~VLFUdpReceiver();

    VLFUdpReceiver(QUdpSocket *udpSocket);

    void slot_receiver_readyRead() override;

    int startReceiving() override;

private:
    QUdpSocket *udp_socket;


};


#endif //VLF_VLFUDPRECEIVER_H
