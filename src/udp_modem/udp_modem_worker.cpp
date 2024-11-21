//
// Created by noisemonitor on 2024/11/15.
//

// You may need to build the project (run Qt uic code generator) to get "ui_udp_modem_worker.h" resolved

#include "udp_modem_worker.h"
#include <qdebug.h>
#include <QThread>
#include <cmath>
#include "liquid.h"

#define UDP_PACKAGE_SIZE 256 // in sample
#define FRAME_SIZE 256+38+35

udp_modem_worker::udp_modem_worker(QObject *parent) :
        QObject(parent){

}

udp_modem_worker::~udp_modem_worker() {

}

void udp_modem_worker::udp_sig_tx() {

    qDebug() << "udp_sig_tx";

// 在次线程中不要写主线程中的值，会有问题
// 按下start按钮同时，还没执行到这行，这时按下stop，再执行下面这行，会覆盖掉quitNow，进入while
// 而这时只有start按钮能用，再次点击，会触发另一个udp_sig_tx的事件加入事件队列，但由于线程卡在前面的while，这个事件不会被处理
// 这时候按stop按钮，会退出第一个while，开始处理第二个udp_sig_tx的事件，将quitNow = false，又进入while
// 从而出现，点击stop按钮，触发start按钮的假象
//    m_config->quitNow = false;


    while (!m_config->quitNow){
        qDebug() << "in loop";
        thread()->sleep(1);
    }

    // global
    int fsa = m_config->wave_config_vec[0].sample_rate;
    double noise_power = m_config->noiseConfig.noise_power_allband;
    static bool is_sig_ch1 = false; // false:noise
    static int tx_state_ch1 = 0;
    float sig_sum[UDP_PACKAGE_SIZE];
    float sig_ch1[UDP_PACKAGE_SIZE];
    int32_t sig_tx[UDP_PACKAGE_SIZE];
    // ch1

    int init_delay_ch1 = m_config->wave_config_vec[0].init_delay * fsa / 1000; // in sample
    int fsy_ch1 = m_config->wave_config_vec[0].symbol_rate;
    int sps_ch1 = fsa / fsy_ch1;
    int siglen_ch1 = FRAME_SIZE * sps_ch1;
    int internal_ch1 = m_config->wave_config_vec[0].wave_internal * fsa / 1000;
    int period_ch1 = siglen_ch1 + internal_ch1;
    if(m_config->wave_config_vec[0].wave_type == "FSK"){

        // 产生FSK信号，放到一个int32/16数组里面
        // tx_state_ch1 = 0噪声 1信号 2间隔

        int i = 0;
        for (; !is_sig_ch1 && i < UDP_PACKAGE_SIZE; i++) {
            sig_ch1[i] = 0;
            if (++tx_state_ch1 >= init_delay_ch1) {
                // 下一个点是信号，状态重置
                is_sig_ch1 = true;
                tx_state_ch1 = 0;
            }
        }

        for (; is_sig_ch1 && i < UDP_PACKAGE_SIZE; i++) {
            if (tx_state_ch1 < siglen_ch1) {
                // 发信号

            } else {
                // 发噪声
                sig_ch1[i] = 0;
            }
            tx_state_ch1 = (tx_state_ch1 + 1) % period_ch1;
        }





    } else if(m_config->wave_config_vec[0].wave_type == "MSK"){

    }

    QString wave_type_ch1 = m_config->wave_config_vec[0].wave_type;
    float avg_power_ch1 = m_config->wave_config_vec[0].avg_power;
    int carrier_freq_ch1 = m_config->wave_config_vec[0].carrier_freq;


    // ch2


    // noise
    for(int i = 0; i < UDP_PACKAGE_SIZE; i++){
        sig_sum[i] = randnf(); // + sig_ch1[i] + sig_ch2[i] + ...
    }


    // AGC



    // udp socket



}

void udp_modem_worker::setMConfig(udp_wave_config *mConfig) {
    m_config = mConfig;
}

void udp_modem_worker::udp_sig_stop() {
    qDebug() << "stop loop";
}


