//
// Created by noisemonitor on 2024/12/16.
//

#ifndef VLF_VLFCHANNEL_H
#define VLF_VLFCHANNEL_H

#include <QObject>


class VLFChannel : public QObject {
Q_OBJECT

public:
    explicit VLFChannel(QObject *parent = nullptr);
    VLFChannel(int idx_ch);


    ~VLFChannel();

public slots:
    void slot_device_info_update(quint32 year_month_day_n, float longitude_n, float latitude_n, float altitude_n);

    void slot_business_package_push(quint8 idx_ch, QSharedPointer<QByteArray> ptr_package);

private:

    // 设备级参数
    uint32_t year_month_day;
    float longitude;
    float latitude;
    float altitude;

    // 通道级参数
    int open_state; // 0:关闭，1:开启

    uint8_t channel_id;
    uint8_t data_type; // 0:cplx, 1:real
    uint16_t save_type; // 0:int16, 1:int32
    uint32_t sample_rate;
    uint32_t freq_lower_edge;
    uint32_t freq_upper_edge;

    QByteArray last_channel_params;

    uint32_t last_udp_idx;





};


#endif //VLF_VLFCHANNEL_H
