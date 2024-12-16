//
// Created by noisemonitor on 2024/12/16.
//


#include "vlfabstractreceiver.h"
#include <QDebug>
#include <QtEndian>


VLFAbstractReceiver::VLFAbstractReceiver(QObject *parent) :
        QObject(parent) {

    vlf_ch = nullptr;
}

VLFAbstractReceiver::~VLFAbstractReceiver() {

}


void VLFAbstractReceiver::process_package(const QByteArray byte_array){

    int byte_size = byte_array.size();
    if(byte_size < 500){ // 状态包，更新设备参数


    device_state = qToBigEndian(*reinterpret_cast<const uint32_t*>((byte_array.mid(20, 4).constData())));
    year_month_day = qToBigEndian(*reinterpret_cast<const uint32_t*>((byte_array.mid(24, 4).constData())));
    longitude = qToBigEndian(*reinterpret_cast<const float*>((byte_array.mid(28, 4).constData())));
    latitude = qToBigEndian(*reinterpret_cast<const float*>((byte_array.mid(32, 4).constData())));
    altitude = qToBigEndian(*reinterpret_cast<const float*>((byte_array.mid(36, 4).constData())));
//    ch0_state = *reinterpret_cast<uint8_t *>((byte_array.at(40)));
//    ch1_state = *reinterpret_cast<uint8_t *>((byte_array.at(41)));
//    ch2_state = *reinterpret_cast<uint8_t *>((byte_array.at(42)));
//    ch3_state = *reinterpret_cast<uint8_t *>((byte_array.at(43)));

    qDebug() << "device:" << device_state;
    qDebug() << "device:" << year_month_day;
    qDebug() << "device:" << longitude;
    qDebug() << "device:" << latitude;
    qDebug() << "device:" << altitude;
    qDebug() << "device:" << ch0_state;
    qDebug() << "device:" << ch1_state;
    qDebug() << "device:" << ch2_state;
    qDebug() << "device:" << ch3_state;



    }else{
        qDebug() << "business package byte size: " << ((uint16_t)byte_array.at(14) << 8 | byte_array.at(15));

    }

}

void VLFAbstractReceiver::set_vlf_ch(VLFChannel *ch) {
    vlf_ch = ch;
}

