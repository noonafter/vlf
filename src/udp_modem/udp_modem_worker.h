//
// Created by noisemonitor on 2024/11/15.
//

#ifndef VLF_UDP_MODEM_WORKER_H
#define VLF_UDP_MODEM_WORKER_H

#include <QObject>
#include <QUdpSocket>
#include <QDataStream>
#include <QFile>
#include "udp_wave_config.h"
#include "fsk_vlf.h"
#include "msk_vlf.h"

#define UDP_SAMPLE_SIZE 256 // in sample
#define FRAME_SIZE (256+38+35)

// 将待发送的udp包备份到本地文件，方便测试
#define UDP_LOCAL_BACKUP
#undef  UDP_LOCAL_BACKUP

struct udp_pac_header {
    uint32_t idx_pac;
    uint8_t check_pac;
    uint8_t version_stcp;
    uint8_t control_trans;
    uint8_t indicator_cache;
    uint32_t ack_recv;
    uint16_t app_type;
    uint16_t pac_len;
};

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
    void udp_tx_business();
    void udp_tx_status();
    void udp_sig_stop();

private:
    udp_wave_config *m_config;

    struct udp_pac_header hd_business;
    struct udp_pac_header hd_status;

    int ch_size;
    bool *is_sig_ch;
    int *tx_sample_ch;
    float (*sig_ch)[UDP_SAMPLE_SIZE];
    float *sig_sum;
    int32_t *sig_tx;

    agc_rrrf agc;
    QUdpSocket udp_send;
    QByteArray bytea;
    QDataStream dstream;

    int package_count = 0;

#ifdef UDP_LOCAL_BACKUP
    QFile *file;
    QDataStream *out;
#endif

    int chx_generate_one_package_bfsk(bfsk_vlf_s * sig_gene_ch1,
                                       bool &is_sig_chx, int &tx_sample_chx, int &init_delay_chx,
                                       int &siglen_chx,  int &period_chx,  float *sig_chx);

    int chx_generate_one_package_msk(msk_vlf_s * sig_gene_ch1,
                                      bool &is_sig_chx, int &tx_sample_chx, int &init_delay_chx,
                                      int &siglen_chx,  int &period_chx,  float *sig_chx);
};


#endif //VLF_UDP_MODEM_WORKER_H
