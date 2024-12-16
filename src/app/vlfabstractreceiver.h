//
// Created by noisemonitor on 2024/12/16.
//

#ifndef VLF_VLFABSTRACTRECEIVER_H
#define VLF_VLFABSTRACTRECEIVER_H

#include <QObject>
#include "vlfchannel.h"
#define CHANNEL_COUNT (4)

// VLFAbstractReceiver屏蔽不同数据采集设备区别，统一数据接收处理逻辑。
// 同时将设备级参数汇总在一起，提供统一的数据访问接口
class VLFAbstractReceiver : public QObject {
Q_OBJECT

public:
    explicit VLFAbstractReceiver(QObject *parent = nullptr);

    // 注册包接收函数，开始监听数据端口
    virtual int startReceiving() = 0;

    // 包接收函数，获取包数据后，统一调用process_package进行包处理（得到包数据后，统一成QByteArray，包处理逻辑是相同的）
    virtual void slot_receiver_readyRead() = 0;

    // 包处理函数，判断是status包还是business包，更新设备级参数，并推送至对应通道
    void process_package(const QByteArray byte_array);

    // 通道注册
    void set_vlf_ch(VLFChannel *ch);

    ~VLFAbstractReceiver() override;

private:
    VLFChannel *vlf_ch;

    uint32_t device_state;
    uint32_t year_month_day;
    float longitude;
    float latitude;
    float altitude;
    uint8_t ch0_state;
    uint8_t ch1_state;
    uint8_t ch2_state;
    uint8_t ch3_state;




};


#endif //VLF_VLFABSTRACTRECEIVER_H
