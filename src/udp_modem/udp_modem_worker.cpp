//
// Created by noisemonitor on 2024/11/15.
//

// You may need to build the project (run Qt uic code generator) to get "ui_udp_modem_worker.h" resolved

#include "udp_modem_worker.h"
#include <qdebug.h>
#include <QThread>
#include <cmath>


#include "liquid.h"





udp_modem_worker::udp_modem_worker(QObject *parent) :
        QObject(parent), udp_send(this), bytea(), dstream(&bytea, QIODevice::WriteOnly) {

    ch_size = 0;
    is_sig_ch = NULL;
    tx_sample_ch = NULL;
    sig_ch = NULL;
    sig_sum = NULL;

    agc = agc_rrrf_create();

    file = new QFile(R"(E:\project\vlf\src\udp_modem\sig_sum_int_tx)");
    out = new QDataStream(file);
    out->setFloatingPointPrecision(QDataStream::SinglePrecision);
    out->setByteOrder(QDataStream::LittleEndian);


}

udp_modem_worker::~udp_modem_worker() {


    agc_rrrf_destroy(agc);
    delete[] sig_ch;
    delete[] sig_sum;
    delete[] is_sig_ch;
    delete[] tx_sample_ch;
    delete[] sig_tx;

    delete file;
    delete out;
}

void udp_modem_worker::udp_sig_tx() {

    qDebug() << "udp_sig_tx";


// 在次线程中不要写自己的控制开关，会有问题，控制开关应该由用户在主线程中控制
// 按下start按钮同时，还没执行到这行，这时按下stop，再执行下面这行，会覆盖掉quitNow，进入while
// 而这时只有start按钮能用，再次点击，会触发另一个udp_sig_tx的事件加入事件队列，但由于线程卡在前面的while，这个事件不会被处理
// 这时候按stop按钮，会退出第一个while，开始处理第二个udp_sig_tx的事件，将quitNow = false，又进入while
// 从而出现，点击stop按钮，触发start按钮的假象
//    m_config->quitNow = false;

// 根据配置初始化环境
    // global
    double noise_power = m_config->noiseConfig.noise_power_allband;
    float std_noise = powf(10, (float)noise_power/20);
    float tmp;

    QHostAddress dest_addr(m_config->udpConfig.dest_ip);
    quint16 dest_port = m_config->udpConfig.dest_port;
//    udp_send.bind(QHostAddress::AnyIPv4, m_config->udpConfig.local_port);
    if(udp_send.bind(QHostAddress::AnyIPv4, m_config->udpConfig.local_port)){
        qDebug() << "bind fail" << udp_send.errorString();
//        return;
    }else {
        qDebug() << "bind success";
    };

    QVector<WaveConfig> &wave_vec = m_config->wave_config_vec;
    QVector<int> &channel_vec = m_config->channelConfig.channels;

    int fc_ch[ch_size];
    int fsep_ch[ch_size];
    int fsa_ch[ch_size];
    int fsy_ch[ch_size];
    int sps_ch[ch_size];
    int init_delay_ch[ch_size];
    int siglen_ch[ch_size];
    int internal_ch[ch_size];
    int period_ch[ch_size];
    float std_ch[ch_size];
    bfsk_vlf_s *fsk_generator_ch[ch_size];
    msk_vlf_s *msk_generator_ch[ch_size];
    agc_rrrf_reset(agc);

    for (int i = 0; i < ch_size; ++i) {
        // chx 信号产生器初始化所需参数
        fc_ch[i] = wave_vec[i].carrier_freq;
        fsep_ch[i] = wave_vec[i].wave_param1;
        fsa_ch[i] = wave_vec[i].sample_rate;
        fsy_ch[i] = wave_vec[i].symbol_rate;
        sps_ch[i] = fsa_ch[i] / fsy_ch[i];
        // chx 产生一包信号所需参数
        init_delay_ch[i] = wave_vec[i].init_delay * fsa_ch[i] / 1000;
        siglen_ch[i] = FRAME_SIZE * sps_ch[i];
        internal_ch[i] = wave_vec[i].wave_internal * fsa_ch[i] / 1000;
        period_ch[i] = siglen_ch[i] + internal_ch[i];
        std_ch[i] = powf(10, (float) wave_vec[i].avg_power / 20);
        // 初始化chx信号产生器
        fsk_generator_ch[i] = NULL;
        msk_generator_ch[i] = NULL;
        if (channel_vec[i] && wave_vec[i].wave_type == "FSK") {
            fsk_generator_ch[i] = bfsk_vlf_create(fc_ch[i], fsep_ch[i], sps_ch[i], fsa_ch[i], FRAME_SIZE);
            bfsk_vlf_frame_in(fsk_generator_ch[i], NULL, FRAME_SIZE);
        } else if (channel_vec[i] && wave_vec[i].wave_type == "MSK") {
            msk_generator_ch[i] = msk_vlf_create(fc_ch[i], sps_ch[i], fsep_ch[i], fsa_ch[i], FRAME_SIZE);
            msk_vlf_frame_in(msk_generator_ch[i], NULL, FRAME_SIZE);
        }
    }


    // 信号循环
    if(!file->open(QIODevice::WriteOnly | QIODevice::Append)){
        qWarning() << "failed to open file" << file->errorString();
        return;
    }
    int idx_package = 0;
    while (!m_config->quitNow && idx_package < 10000) {
        qDebug() << "in loop";

        // chx
        for (int i = 0; i < ch_size; i++) {
            if (channel_vec[i] && wave_vec[i].wave_type == "FSK") {
                // 生成一包FSK信号（256点）
                chx_generate_one_package_bfsk(fsk_generator_ch[i], is_sig_ch[i], tx_sample_ch[i], init_delay_ch[i],
                                              siglen_ch[i], period_ch[i], sig_ch[i]);
            } else if (channel_vec[i] && wave_vec[i].wave_type == "MSK") {
                // 生成一包MSK信号（256点）
                chx_generate_one_package_msk(msk_generator_ch[i], is_sig_ch[i], tx_sample_ch[i], init_delay_ch[i],
                                             siglen_ch[i], period_ch[i], sig_ch[i]);
            } else {
                memset(sig_ch[i], 0, sizeof(sig_ch[i]));
            }
        }

        for (int j = 0; j < UDP_PACKAGE_SIZE; j++) {
            sig_sum[j] = 0;
            // noise
            for (int i = 0; i < ch_size; i++) {
                sig_sum[j] += std_noise * randnf() + std_ch[i] * sig_ch[i][j]; // randnf() + sig_ch[i]][j] ...
            }

            // AGC
            agc_rrrf_execute(agc, sig_sum[j], &tmp);

            // udp socket
            sig_tx[j] = (int32_t)(tmp * pow(2,29));
            *out << sig_tx[j];
            dstream << sig_tx[j];
        }

//        qDebug() << "bytea len" << bytea.size();
        udp_send.writeDatagram(bytea, dest_addr, dest_port);
        udp_send.close();
        bytea.clear();
        dstream.device()->seek(0);


//        QThread::msleep(1);


        idx_package++;
    } // while end


    file->close();

    for (int i = 0; i < ch_size; i++) {
        bfsk_vlf_destroy(fsk_generator_ch[i]);
        msk_vlf_destroy(msk_generator_ch[i]);
    }


}

void udp_modem_worker::setMConfig(udp_wave_config *mConfig) {
    m_config = mConfig;

    ch_size = m_config->wave_config_vec.size();

    sig_ch = new float[ch_size][UDP_PACKAGE_SIZE];
    sig_sum = new float[UDP_PACKAGE_SIZE];
    is_sig_ch = new bool[ch_size];
    tx_sample_ch = new int[ch_size];
    sig_tx = new int32_t[UDP_PACKAGE_SIZE];

    memset(sig_ch, 0, ch_size * UDP_PACKAGE_SIZE * sizeof(float));
    memset(sig_sum, 0, UDP_PACKAGE_SIZE*sizeof(float));
    memset(is_sig_ch, 0, ch_size * sizeof(bool));
    memset(tx_sample_ch, 0, ch_size * sizeof(int));

}

void udp_modem_worker::udp_sig_stop() {
    qDebug() << "stop loop";
}

int udp_modem_worker::chx_generate_one_package_bfsk(bfsk_vlf_s *sig_gene_ch1,
                                                    bool &is_sig_chx, int &tx_sample_chx, int &init_delay_chx,
                                                    int &siglen_chx,  int &period_chx,  float *sig_chx) {
    if(!sig_gene_ch1)
        return 0;

    // is_sig_chx用来监控是否是初始延迟
    // tx_sample_ch1用来监控是信号还是间隔
    int i = 0;
    // 进入初始延迟，信号为0
    for (; !is_sig_chx && i < UDP_PACKAGE_SIZE; i++) {
        sig_chx[i] = 0;
        // 下一个点是信号，状态重置
        if (++tx_sample_chx >= init_delay_chx) {
            is_sig_chx = true;
            tx_sample_chx = 0;
        }
    }
    // 进入信号/间隔
    for (; is_sig_chx && i < UDP_PACKAGE_SIZE; i++) {
        // 信号
        if (tx_sample_chx < siglen_chx) {
            // 这里if不能合并，否则大部分情况都会进入间隔，将信号覆盖了
            if( bfsk_vlf_modulate_block(sig_gene_ch1, sig_chx + i, 1))
                bfsk_vlf_frame_in(sig_gene_ch1, NULL, FRAME_SIZE);
            // 实数cos信号，能量为0.5
            sig_chx[i] *= sqrtf(2.0f);
        // 间隔
        } else {
            sig_chx[i] = 0;
        }
        tx_sample_chx = (tx_sample_chx + 1) % period_chx;
    }


    return 0;
}

int udp_modem_worker::chx_generate_one_package_msk(msk_vlf_s *sig_gene_ch1,
                                                   bool &is_sig_chx, int &tx_sample_chx, int &init_delay_chx,
                                                   int &siglen_chx, int &period_chx, float *sig_chx) {
    if(!sig_gene_ch1)
        return 0;

    // is_sig_chx用来监控是否是初始延迟
    // tx_sample_ch1用来监控是信号还是间隔
    int i = 0;
    // 进入初始延迟，信号为0
    for (; !is_sig_chx && i < UDP_PACKAGE_SIZE; i++) {
        sig_chx[i] = 0;
        // 下一个点是信号，状态重置
        if (++tx_sample_chx >= init_delay_chx) {
            is_sig_chx = true;
            tx_sample_chx = 0;
        }
    }
    // 进入信号/间隔
    for (; is_sig_chx && i < UDP_PACKAGE_SIZE; i++) {
        // 信号
        if (tx_sample_chx < siglen_chx) {
            // 这里if不能合并，否则大部分情况都会进入间隔，将信号覆盖了
            if( msk_vlf_modulate_block(sig_gene_ch1, sig_chx + i, 1))
                msk_vlf_frame_in(sig_gene_ch1, NULL, FRAME_SIZE);
            // 实数cos信号，能量为0.5
            sig_chx[i] *= sqrtf(2.0f);
            // 间隔
        } else {
            sig_chx[i] = 0;
        }
        tx_sample_chx = (tx_sample_chx + 1) % period_chx;
    }


    return 0;


}





