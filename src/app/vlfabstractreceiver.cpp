//
// Created by noisemonitor on 2024/12/16.
//


#include "vlfabstractreceiver.h"
#include <QDebug>
#include <QtEndian>


VLFAbstractReceiver::VLFAbstractReceiver(QObject *parent) :
        QObject(parent) {

    m_chs = nullptr;
    m_config = nullptr;

}

VLFAbstractReceiver::~VLFAbstractReceiver() {

}


void VLFAbstractReceiver::process_package(const QByteArray &byte_array){

    int byte_size = byte_array.size();
    // 这里的一个接收机，四个通道，实际上是4个单一生产者&单一消费者的模型
    // 如果这里采用智能指针+信号槽的方式进行数据传递，实际上相当于开了一个可用内存大小的队列，这种做有几个缺点：
    // 1 队列大小相当于堆的大小，如果生产者速度过快，或者消费者卡顿，很容易导致堆内存溢出，从而导致new失败抛出异常.当然可以用一些其他的办法来限制队列大小，比如对同类智能指针个数进行计数并限制
    // 2 消费者取数据时，只能保持与生产者同样的数据大小，后续如果需要一次读取更多的数据，还是需要自己写一个缓存
    //综合以上，为了防止潜在的堆内存溢出的风险，并且方便消费者取任意长数据，考虑使用网上现有的单一生产者&单一消费者队列
    QSharedPointer<QByteArray> p_package =  QSharedPointer<QByteArray>(new QByteArray(byte_array));

    if(byte_size < 500 && byte_size >= 44){ // 状态包，更新设备参数

        if (byte_array.at(16) == (char)0x7e && byte_array.at(17) == (char)0xef ) {
            VLFDeviceConfig *d_config = &m_config->device_config;
            d_config->device_state = qToBigEndian(*reinterpret_cast<const uint32_t *>((byte_array.mid(20, 4).constData())));
            d_config->year_month_day = qToBigEndian(*reinterpret_cast<const uint32_t *>((byte_array.mid(24, 4).constData())));
            d_config->longitude = qToBigEndian(*reinterpret_cast<const float *>((byte_array.mid(28, 4).constData())));
            d_config->latitude = qToBigEndian(*reinterpret_cast<const float *>((byte_array.mid(32, 4).constData())));
            d_config->altitude = qToBigEndian(*reinterpret_cast<const float *>((byte_array.mid(36, 4).constData())));
            d_config->ch0_state = (uint8_t) (byte_array.at(40));
            d_config->ch1_state = (uint8_t) (byte_array.at(41));
            d_config->ch2_state = (uint8_t) (byte_array.at(42));
            d_config->ch3_state = (uint8_t) (byte_array.at(43));

            // 通知各个通道，部分设备信息
            emit signal_device_info_updated(*d_config);
        }

    }else if(byte_size < 1080){ // 业务包

        if(byte_array.at(28) == (char)0x7e && byte_array.at(29) == (char)0x03){
            quint8 idx_ch = (quint8) (byte_array.at(32));
//            qDebug() << "idx_ch: " << idx_ch;
             emit signal_business_package_ready(idx_ch, p_package);
        }

    }

}

void VLFAbstractReceiver::set_vlf_ch(QVector<VLFChannel *> *chs) {
    m_chs = chs;
    for (auto ch: *m_chs) {
        connect(this, &VLFAbstractReceiver::signal_device_info_updated, ch, &VLFChannel::slot_device_info_update);
        connect(this, &VLFAbstractReceiver::signal_channel_info_updated, ch, &VLFChannel::slot_channel_info_update);
        connect(this, &VLFAbstractReceiver::signal_business_package_ready, ch, &VLFChannel::slot_business_package_push);
    }
}

void VLFAbstractReceiver::set_vlf_config(VLFReceiverConfig *config) {
    m_config = config;
    emit signal_device_info_updated(m_config->device_config);
    for (int i = 0; i < CHANNEL_COUNT; ++i) {
        emit signal_channel_info_updated(i, m_config->ch_config_vec[i]);
    }
}

