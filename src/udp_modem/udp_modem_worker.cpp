//
// Created by noisemonitor on 2024/11/15.
//

// You may need to build the project (run Qt uic code generator) to get "ui_udp_modem_worker.h" resolved

#include "udp_modem_worker.h"
#include <qdebug.h>
#include <QThread>

udp_modem_worker::udp_modem_worker(QObject *parent) :
        QObject(parent){

}

udp_modem_worker::~udp_modem_worker() {

}

void udp_modem_worker::udp_sig_tx() {

    qDebug() << "udp_sig_tx";
    float fsa = m_config->wave_config_vec[0].sample_rate;
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

    // ch1


    // ch2


    // noise


    // AGC



    // udp socket



}

void udp_modem_worker::setMConfig(udp_wave_config *mConfig) {
    m_config = mConfig;
}

void udp_modem_worker::udp_sig_stop() {
    qDebug() << "stop loop";
}


