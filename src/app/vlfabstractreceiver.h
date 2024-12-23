//
// Created by noisemonitor on 2024/12/16.
//

#ifndef VLF_VLFABSTRACTRECEIVER_H
#define VLF_VLFABSTRACTRECEIVER_H

#include <QObject>
#include "vlfchannel.h"
#include <QTimer>
#include "VLFReceiverConfig.h"


// VLFAbstractReceiver
// 1 屏蔽不同数据采集设备区别，统一数据接收处理逻辑，比如处理后分发到4个通道。
// 2 同时将设备参数（包括通道参数）汇总在一起，提供统一的数据访问接口（内部类）
class VLFAbstractReceiver : public QObject {
Q_OBJECT

public:
    explicit VLFAbstractReceiver(QObject *parent = nullptr);

    // 注册包接收函数，开始监听数据端口
    virtual int startReceiving() = 0;

    // 包接收函数，获取包数据后，统一调用process_package进行包处理（得到包数据后，统一成QByteArray，包处理逻辑是相同的）
    virtual void slot_receiver_readyRead() = 0;

    // 包处理函数，判断是status包还是business包，更新设备级参数，并推送至对应通道
    void process_package(const QByteArray &byte_array);

    // 通道注册
    void set_vlf_ch(QVector<VLFChannel*> *chs);
    // 配置类注册
    void set_vlf_config(VLFReceiverConfig* config);

    ~VLFAbstractReceiver() override;

signals:
    void signal_device_info_updated(VLFDeviceConfig d_config);
    void signal_channel_info_updated(quint8 idx_ch, VLFChannelConfig ch_config);
    void signal_business_package_enqueued(quint8 idx_ch);

private:
    QVector<VLFChannel *> *m_chs;

    // 将设备参数和所有通道参数，都装到配置类中，提供统一的数据访问接口
    VLFReceiverConfig* m_config;

};


#endif //VLF_VLFABSTRACTRECEIVER_H
