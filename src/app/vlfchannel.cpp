//
// Created by noisemonitor on 2024/12/16.
//

// You may need to build the project (run Qt uic code generator) to get "ui_VLFChannel.h" resolved

#include "vlfchannel.h"
#include <QDebug>
#include <QThread>

VLFChannel::VLFChannel(QObject *parent) : QObject(parent) {

    is_init = false;

}

VLFChannel::~VLFChannel() {

    qDebug() << "id is " << QThread::currentThreadId();
    if(this)
    {
        qDebug() << "has " << this->is_init;
    }else{
        qDebug() << "this null ";
    }

}

void VLFChannel::push_status_package(QByteArray &byte_array) {

    qDebug() << "push_status_package";
}

void VLFChannel::push_business_package(QByteArray &byte_array) {

    qDebug() << "push_business_package";

}

