//
// Created by noisemonitor on 2024/12/16.
//

// You may need to build the project (run Qt uic code generator) to get "ui_VLFChannel.h" resolved

#include "vlfchannel.h"
#include <QDebug>
#include <QThread>
#include <QtEndian>
#include <QDataStream>

VLFChannel::VLFChannel(QObject *parent) : QObject(parent){

    m_queue = ReaderWriterQueue<QByteArray>(512);
    ch_info.open_state = true;

    last_channel_params = QByteArray(16,'\0');
    last_udp_idx = 0;

    recv_count = 0;

}

VLFChannel::VLFChannel(int idx)  {

    ch_info.channel_id = idx;
    ch_info.open_state = true;

    last_channel_params = QByteArray(16,'\0');
    last_udp_idx = 0;
}

VLFChannel::~VLFChannel() {

    qDebug() << "id is " << QThread::currentThreadId();
    if(this)
    {
        qDebug() << "has " << this;
    }else{
        qDebug() << "this null ";
    }

}

bool VLFChannel::package_enqueue(const QByteArray &package) {
    return m_queue.try_enqueue(package);
}

void VLFChannel::slot_device_info_update(VLFDeviceConfig d_config) {

    d_info = d_config;

}

void VLFChannel::slot_channel_info_update(VLFChannelConfig ch_config) {

    ch_info = ch_config;
    last_udp_idx = 0;
    QDataStream out(&last_channel_params,QIODevice::WriteOnly);
    out << ch_info.channel_id
        << ch_info.data_type
        << ch_info.save_type
        << ch_info.sample_rate
        << ch_info.freq_lower_edge
        << ch_info.freq_upper_edge;
}



void VLFChannel::slot_business_package_enqueued() {

    QByteArray package;
    if(!m_queue.try_dequeue(package)){
        return;
    }

    if (!ch_info.open_state || package.size() < 1076) {
        return;
    }

    // 收到业务包后，如果是第一次收到，初始化通道参数；如果不是第一次收到，检查参数与本地是否一致
    // 这个逻辑不对，应该是直接与本地参数进行比较，不一致直接丢弃，不存在下位机改变上位机参数
    if(last_channel_params != package.mid(32,16)){
        return;
    }

    uint32_t cnt_udp_idx = qToBigEndian(*reinterpret_cast<const uint32_t *>((package.mid(0, 4).constData())));
    // 如果udp包号小于上一次的号且号码邻近，说明这一包晚到了，直接丢掉
    if (cnt_udp_idx <= last_udp_idx && last_udp_idx - cnt_udp_idx < 1<<20) {
        return;
    }
    last_udp_idx = cnt_udp_idx;

//    static int recv_count = 0;
    recv_count++;
    if(!(recv_count%500)){
        qDebug() << "drop_count:" << recv_count;
    }

}







