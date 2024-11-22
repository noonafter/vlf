//
// Created by noisemonitor on 2024/11/15.
//

#ifndef VLF_UDP_MODEM_WORKER_H
#define VLF_UDP_MODEM_WORKER_H

#include <QObject>
#include "udp_wave_config.h"
#include "fsk_vlf.h"

class udp_modem_widget;

class udp_modem_worker : public QObject {
Q_OBJECT

public:
    explicit udp_modem_worker(QObject *parent = nullptr);

//这里有点问题，如果参数为udp_modem_widget*，那调用时，具体匹配的是哪个构造器呢？
//    udp_modem_worker(udp_modem_widget *widget);

    ~udp_modem_worker() override;

    void setMConfig(udp_wave_config *mConfig);

public slots:
    void udp_sig_tx();
    void udp_sig_stop();

private:
    udp_wave_config *m_config;
    int tx_sample_ch1;
    bool is_sig_ch1;

    int chx_generate_one_package_bfsk(bfsk_vlf_s * sig_gene_ch1,
                                       bool &is_sig_chx, int &tx_sample_chx, int &init_delay_chx,
                                       int &siglen_chx,  int &period_chx,  float *sig_chx);
};


#endif //VLF_UDP_MODEM_WORKER_H
