//
// Created by noisemonitor on 2024/12/16.
//

#ifndef VLF_VLFCHANNEL_H
#define VLF_VLFCHANNEL_H

#include <QObject>
#include "VLFReceiverConfig.h"
#include "readerwriterqueue.h"

using namespace moodycamel;

class VLFChannel : public QObject {
Q_OBJECT

public:
    explicit VLFChannel(QObject *parent = nullptr);
    VLFChannel(int idx_ch);



    ~VLFChannel();

public slots:
    void slot_device_info_update(VLFDeviceConfig d_config);
    void slot_channel_info_update(quint8 idx_ch, VLFChannelConfig ch_config);

    void slot_business_package_push(quint8 idx_ch, QSharedPointer<QByteArray> ptr_package);

private:


    QByteArray last_channel_params;
    uint32_t last_udp_idx;

    // 设备级参数
    VLFDeviceConfig d_info;
    // 通道级参数
    VLFChannelConfig ch_info;

};


#endif //VLF_VLFCHANNEL_H
