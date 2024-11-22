//
// Created by noisemonitor on 2024/11/15.
//

// You may need to build the project (run Qt uic code generator) to get "ui_udp_modem_worker.h" resolved

#include "udp_modem_worker.h"
#include <qdebug.h>
#include <QThread>
#include <cmath>
#include <QFile>
#include <QDataStream>
#include "liquid.h"
#include "fsk_vlf.h"

#define UDP_PACKAGE_SIZE 256 // in sample
#define FRAME_SIZE (256+38+35)

udp_modem_worker::udp_modem_worker(QObject *parent) :
        QObject(parent){

    tx_sample_ch1 = 0;
    is_sig_ch1 = false;

}

udp_modem_worker::~udp_modem_worker() {

}

void udp_modem_worker::udp_sig_tx() {

    qDebug() << "udp_sig_tx";
    QFile file(R"(E:\project\vlf\src\udp_modem\sig_sum)");
    if(!file.open(QIODevice::WriteOnly | QIODevice::Append)){
        qWarning() << "failed to open file" << file.errorString();
        return;
    }
    QDataStream out(&file);
    out.setFloatingPointPrecision(QDataStream::SinglePrecision);
    out.setByteOrder(QDataStream::LittleEndian);

// 在次线程中不要写主线程中的值，会有问题
// 按下start按钮同时，还没执行到这行，这时按下stop，再执行下面这行，会覆盖掉quitNow，进入while
// 而这时只有start按钮能用，再次点击，会触发另一个udp_sig_tx的事件加入事件队列，但由于线程卡在前面的while，这个事件不会被处理
// 这时候按stop按钮，会退出第一个while，开始处理第二个udp_sig_tx的事件，将quitNow = false，又进入while
// 从而出现，点击stop按钮，触发start按钮的假象
//    m_config->quitNow = false;

// 根据配置初始化环境
    // global
    double noise_power = m_config->noiseConfig.noise_power_allband;
    float sig_sum[UDP_PACKAGE_SIZE] = {0.0f};
    float sig_ch1[UDP_PACKAGE_SIZE] = {0.0f};
    int32_t sig_tx[UDP_PACKAGE_SIZE] = {0};
    // ch1
    WaveConfig wave1 = m_config->wave_config_vec[0];
    int fsa_ch1 = wave1.sample_rate;
    int init_delay_ch1 = wave1.init_delay * fsa_ch1 / 1000; // in sample
    int fsy_ch1 = wave1.symbol_rate;
    int sps_ch1 = fsa_ch1 / fsy_ch1;
    int fsep_ch1 = wave1.wave_param1;
    int fc_ch1 = wave1.carrier_freq;
    int siglen_ch1 = FRAME_SIZE * sps_ch1;
    int internal_ch1 = wave1.wave_internal * fsa_ch1 / 1000;
    int period_ch1 = siglen_ch1 + internal_ch1;
    bfsk_vlf_s *sig_gene_ch1 = NULL;

    if(m_config->channelConfig.channels[0] && wave1.wave_type == "FSK"){
        sig_gene_ch1 = bfsk_vlf_create(fc_ch1, fsep_ch1, sps_ch1, fsa_ch1);
    }


    int idx_package = 0;
    while (!m_config->quitNow && idx_package < 100) {
        qDebug() << "in loop";

        // ch1
        if (m_config->channelConfig.channels[0] && wave1.wave_type == "FSK") {
            // 产生FSK信号，放到一个int32/16数组里面
            // tx_sample_ch1 = 0噪声 1信号 2间隔
            int i = 0;
            // 进入初始延迟，信号为0
            for (; !is_sig_ch1 && i < UDP_PACKAGE_SIZE; i++) {
                sig_ch1[i] = 0;
                // 下一个点是信号，状态重置
                if (++tx_sample_ch1 >= init_delay_ch1) {
                    is_sig_ch1 = true;
                    tx_sample_ch1 = 0;
                }
            }
            // 进入信号/间隔
            for (; is_sig_ch1 && i < UDP_PACKAGE_SIZE; i++) {
                if (tx_sample_ch1 < siglen_ch1) {
                    bfsk_vlf_modulate_block(sig_gene_ch1, sig_ch1+i, 1); // 信号
                } else {
                    sig_ch1[i] = 0; // 间隔
                }
                tx_sample_ch1 = (tx_sample_ch1 + 1) % period_ch1;
            }
        } else if (m_config->wave_config_vec[0].wave_type == "MSK") {

        }

        // ch2


        // noise
        for (int i = 0; i < UDP_PACKAGE_SIZE; i++) {
            sig_sum[i] = sig_ch1[i]; // randnf() + + sig_ch1[i] + sig_ch2[i] + ...
            out << sig_sum[i];
//            out << cosf(2*3.141592654*1*i/16);

        }


        // AGC



        // udp socket



        idx_package++;
    } // while end



//    float avg_power_ch1 = m_config->wave_config_vec[0].avg_power;
//    int carrier_freq_ch1 = m_config->wave_config_vec[0].carrier_freq;





    file.close();

    bfsk_vlf_destroy(sig_gene_ch1);



}

void udp_modem_worker::setMConfig(udp_wave_config *mConfig) {
    m_config = mConfig;
}

void udp_modem_worker::udp_sig_stop() {
    qDebug() << "stop loop";
}


