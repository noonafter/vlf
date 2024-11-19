//
// Created by noisemonitor on 2024/11/15.
//

#ifndef VLF_UDP_MODEM_WORKER_H
#define VLF_UDP_MODEM_WORKER_H

#include <QObject>



class udp_modem_worker : public QObject {
Q_OBJECT

public:
    explicit udp_modem_worker(QObject *parent = nullptr);

    ~udp_modem_worker() override;

private:

};


#endif //VLF_UDP_MODEM_WORKER_H
