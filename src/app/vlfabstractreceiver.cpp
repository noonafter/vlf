//
// Created by noisemonitor on 2024/12/16.
//


#include "vlfabstractreceiver.h"
#include <QDebug>
#include <QtEndian>


VLFAbstractReceiver::VLFAbstractReceiver(QObject *parent) :
        QObject(parent), m_sender(CHANNEL_COUNT) {
    m_chs = nullptr;
    m_config = nullptr;

    for(int i = 0; i < CHANNEL_COUNT; ++i){
        m_sender[i] = new SignalSender(this);
    }

}

VLFAbstractReceiver::~VLFAbstractReceiver() {

}


void VLFAbstractReceiver::process_package(const QByteArray &byte_array){

    int byte_size = byte_array.size();
    // 这里的一个接收机，四个通道，实际上是4个单一生产者&单一消费者的模型
    // 这里有两种实现方式：1.信号槽；2.并发队列
    // 如果采用信号槽的方式（信号槽+对象，注意不是引用）进行大量数据的传递，虽然qt的cow机制避免了大量数据的复制，但是这种做有几个缺点：
    // 1 信号槽机制实际上相当于开了一个可用内存大小的队列，如果生产者速度过快，或者消费者卡顿，很容易导致堆内存溢出。仍然需要用其他方法辅助进行限制，比如智能指针计数。
    // 2 消费者取数据时，只能保持与生产者同样的数据大小，如果希望一次读取更多的数据，还需要自己写一个缓存
    //综合以上，为了防止潜在的堆内存溢出的风险，并且方便消费者取任意长数据，考虑使用网上现有的并发队列

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
            qDebug() << "receive status package, length is: " << byte_size;
        }

    }else if(byte_size <= 1076 && byte_size >= 500){ // 业务包

        if(byte_array.at(28) == (char)0x7e && byte_array.at(29) == (char)0x03){
            quint8 idx_ch = (quint8) (byte_array.at(32));
            // 尝试转发，如果queue存在空位。转发后，由于qt的cow机制，入队实际上实现了移动拷贝的效果。
            if(m_chs->at(idx_ch)->package_enqueue(byte_array)){
                m_sender[idx_ch]->emit_signal_business_package_enqueued();
            }

            uint32_t stcp_num = qToBigEndian(*reinterpret_cast<const uint32_t *>((byte_array.mid(0, 4).constData())));
            if (!(stcp_num % 7500)) {
                qDebug() << "ch" << idx_ch << "stcp_num:" << stcp_num << "at " << QDateTime::currentDateTime();;
            }
        }

    } else{
        qDebug() << "receive package, but length is: " << byte_size;
    }

}

void VLFAbstractReceiver::set_vlf_ch(QVector<VLFChannel *> *chs) {
    m_chs = chs;
    for (int i = 0; i < CHANNEL_COUNT; ++i) {
        connect(this, &VLFAbstractReceiver::signal_device_info_updated, m_chs->at(i), &VLFChannel::slot_device_info_update);
        connect(m_sender[i], &SignalSender::signal_channel_info_updated, m_chs->at(i), &VLFChannel::slot_channel_info_update);
        connect(m_sender[i], &SignalSender::signal_business_package_enqueued, m_chs->at(i), &VLFChannel::slot_business_package_enqueued);
    }
}

void VLFAbstractReceiver::set_vlf_config(VLFReceiverConfig *config) {
    m_config = config;
    // 接收端启动时，更新一次设备的日期信息，防止生成的文件日期信息不对(默认启动时，本机时间和采集设备的日期相同)
    m_config->device_config.year_month_day = QDate::currentDate().toString("yyyyMMdd").toUInt();
    // 通知每个信道初始化设备级参数
    emit signal_device_info_updated(m_config->device_config);
    // 通知每个信道初始化信道级参数
    for (int i = 0; i < CHANNEL_COUNT; ++i) {
        m_sender[i]->emit_signal_channel_info_updated(m_config->ch_config_vec[i]);
    }
}

