//
// Created by noisemonitor on 2024/11/15.
//

// You may need to build the project (run Qt uic code generator) to get "ui_udp_modem_worker.h" resolved

#include "udp_modem_worker.h"
#include <qdebug.h>


udp_modem_worker::udp_modem_worker(QObject *parent) :
        QObject(parent){

}

//udp_modem_worker::udp_modem_worker(udp_modem_widget *widget) {
//
//    m_widget = widget;
//
//}

udp_modem_worker::~udp_modem_worker() {

}

void udp_modem_worker::udp_tx_sig() {

    qDebug() << "slot_pushButton_start_clicked";




    // ch1


    // ch2


    // noise


    // AGC



    // udp socket



}

void udp_modem_worker::setMWidget(udp_modem_widget *mWidget) {
    m_widget = mWidget;
}

