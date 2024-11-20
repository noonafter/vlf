//
// Created by noisemonitor on 2024/11/15.
//

#ifndef VLF_UDP_MODEM_WORKER_H
#define VLF_UDP_MODEM_WORKER_H

#include <QObject>
//#include <udp_modem_widget.h>

class udp_modem_widget;

class udp_modem_worker : public QObject {
Q_OBJECT

public:
    explicit udp_modem_worker(QObject *parent = nullptr);

//这里有点问题，如果参数为udp_modem_widget*，那调用时，具体匹配的是哪个构造器呢？
//    udp_modem_worker(udp_modem_widget *widget);
public:
    void setMWidget(udp_modem_widget *mWidget);

    ~udp_modem_worker() override;

public slots:
    void udp_tx_sig();

private:
    udp_modem_widget *m_widget;


};


#endif //VLF_UDP_MODEM_WORKER_H
