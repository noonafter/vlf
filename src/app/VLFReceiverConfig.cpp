//
// Created by noisemonitor on 2024/12/19.
//

#include "VLFReceiverConfig.h"
#include <QDateTime>

VLFReceiverConfig::VLFReceiverConfig(const QString &configPath) : configPath(configPath), device_config{}, ch_config_vec(CHANNEL_COUNT) {
    loadConfig();
}

VLFReceiverConfig::~VLFReceiverConfig() {
    saveConfig();
}

bool VLFReceiverConfig::loadConfig() {
    QFile file(configPath);
    if (!file.exists()) {
        createDefaultConfig();
        return saveConfig();
    }
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Cannot open config file for reading:" << configPath;
        return false;
    }

    // file内容读到QByteArray
    QByteArray data = file.readAll();
    file.close();

    // QByteArray内容转成Json doc
    QJsonDocument doc = QJsonDocument::fromJson(data);

    if (doc.isNull() || !doc.isObject()) {
        qWarning() << "Invalid JSON format:" << configPath;
        return false;
    }

    // 产生根obj，基于Json doc
    QJsonObject rootObj = doc.object();
    QJsonObject device_obj = rootObj["device_config"].toObject();
    QJsonArray channel_array = rootObj["channel_config"].toArray();

    // 读device_obj到属性
    device_config.device_state = (uint32_t) device_obj["device_state"].toInt();
    device_config.year_month_day = (uint32_t) device_obj["year_month_day"].toInt();
    device_config.longitude = (float) device_obj["longitude"].toDouble();
    device_config.latitude = (float) device_obj["latitude"].toDouble();
    device_config.altitude = (float) device_obj["altitude"].toDouble();
    device_config.ch0_state = (uint8_t) device_obj["ch0_state"].toInt();
    device_config.ch1_state = (uint8_t) device_obj["ch1_state"].toInt();
    device_config.ch2_state = (uint8_t) device_obj["ch2_state"].toInt();
    device_config.ch3_state = (uint8_t) device_obj["ch3_state"].toInt();

    // 读channel_array到属性
    ch_config_vec.clear();
    for (auto value: channel_array) {
        QJsonObject chObj = value.toObject();
        VLFChannelConfig ch{};
        ch.open_state = chObj["open_state"].toInt();
        ch.channel_id = (uint8_t) chObj["channel_id"].toInt();
        ch.data_type = (uint8_t) chObj["data_type"].toInt();
        ch.save_type = (uint16_t) chObj["save_type"].toInt();
        ch.sample_rate = (uint32_t) chObj["sample_rate"].toInt();
        ch.freq_lower_edge = (uint32_t) chObj["freq_lower_edge"].toInt();
        ch.freq_upper_edge = (uint32_t) chObj["freq_upper_edge"].toInt();
        ch_config_vec.append(ch);
    }
    return true;
}

bool VLFReceiverConfig::saveConfig() {

    // 产生device_config节点
    QJsonObject device_obj{
            {"device_state",   (qint32) device_config.device_state},
            {"year_month_day", (qint32) device_config.year_month_day},
            {"longitude",      device_config.longitude},
            {"latitude",       device_config.latitude},
            {"altitude",       device_config.altitude},
            {"ch0_state",      device_config.ch0_state},
            {"ch1_state",      device_config.ch1_state},
            {"ch2_state",      device_config.ch2_state},
            {"ch3_state",      device_config.ch3_state}
    };
    // 产生channel_config节点
    QJsonArray channel_array;
    for (const auto &ch: ch_config_vec) {
        QJsonObject chObj{
                {"open_state", ch.open_state},
                {"channel_id", ch.channel_id},
                {"data_type", ch.data_type},
                {"save_type", ch.save_type},
                {"sample_rate", (qint32) ch.sample_rate},
                {"freq_lower_edge", (qint32) ch.freq_lower_edge},
                {"freq_upper_edge", (qint32) ch.freq_upper_edge}
        };
        channel_array.append(chObj);
    }

    // 产生要写入的根obj
    QJsonObject rootObj{
            {"device_config", device_obj},
            {"channel_config", channel_array}
    };

    //产生对应的json doc
    QJsonDocument doc(rootObj);

    //使用QFile将json doc写入文件
    QFile file(configPath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Cannot open config file for writing:" << configPath;
        return false;
    }
    file.write(doc.toJson());
    file.close();
    qDebug() << "config write over";
    return true;
}

void VLFReceiverConfig::createDefaultConfig() {

    uint32_t ymd = QDateTime::currentDateTime().toString("yyyyMMdd").toUInt();
    device_config = {0, ymd, 0.0, 0.0, 0.0, 0, 0, 0, 0};

    for(int i = 0; i< CHANNEL_COUNT;++i){
        ch_config_vec[i] = {1, (uint8_t)i, 1, 1, 192000, 10000, 60000};
    }
}


