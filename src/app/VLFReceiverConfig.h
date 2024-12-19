//
// Created by noisemonitor on 2024/12/19.
//

#ifndef VLF_VLFRECEIVERCONFIG_H
#define VLF_VLFRECEIVERCONFIG_H
#include <QString>
#include <QVector>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QFile>
#include <QDebug>

#define CHANNEL_COUNT (4)

// 设备级参数
struct VLFDeviceConfig{
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

// 通道级参数
struct VLFChannelConfig {
    int open_state; // 0:关闭，1:开启
    uint8_t channel_id;
    uint8_t data_type; // 0:cplx, 1:real
    uint16_t save_type; // 0:int16, 1:int32
    uint32_t sample_rate;
    uint32_t freq_lower_edge;
    uint32_t freq_upper_edge;
};

class VLFReceiverConfig {

public:
    explicit VLFReceiverConfig(const QString &configPath);

    virtual ~VLFReceiverConfig();

    bool loadConfig();
    bool saveConfig();
    // 对属性赋一个默认初值
    void createDefaultConfig();

    // 设备级参数
    VLFDeviceConfig device_config;
    // 通道级参数
    QVector<VLFChannelConfig> ch_config_vec;

private:
    QString configPath;

};


#endif //VLF_VLFRECEIVERCONFIG_H
