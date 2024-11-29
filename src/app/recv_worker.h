//
// Created by noisemonitor on 2024/11/28.
//

#ifndef VLF_RECV_WORKER_H
#define VLF_RECV_WORKER_H


#include <QUdpSocket>
#include <QFile>
#include <QDataStream>

class recv_worker : public QObject{

Q_OBJECT

public:
    recv_worker(QObject *parent = nullptr);

    ~recv_worker();

public slots:
    void slot_udp_recv_readyRead();

private:
    QUdpSocket *udp_recv;

    QFile *file;
    QDataStream *out;

    int count_rev;

};


#endif //VLF_RECV_WORKER_H
