//
// Created by noisemonitor on 2024/12/16.
//


#include "vlfabstractreceiver.h"
#include <QDebug>


VLFAbstractReceiver::VLFAbstractReceiver(QObject *parent) :
        QObject(parent) {

}

VLFAbstractReceiver::~VLFAbstractReceiver() {

}


void VLFAbstractReceiver::process_package(const QByteArray &byte_array){


    int byte_size = byte_array.size();
    if(byte_size < 500){

        qDebug() << "status package byte size: " << ((uint16_t)byte_array.at(14) << 8 | byte_array.at(15));

    }else{
        qDebug() << "business package byte size: " << ((uint16_t)byte_array.at(14) << 8 | byte_array.at(15));

    }

}

