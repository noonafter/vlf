//
// Created by noisemonitor on 2024/11/20.
//


#include "udp_wave_config.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDebug>
#include <QNetworkInterface>
#include <QList>

udp_wave_config::udp_wave_config(QString file_path) {

    configPath = file_path;
    quitNow = true;

}

udp_wave_config::~udp_wave_config() {
    quitNow = true;
}

QString udp_wave_config::getLocalIPAddress() {

    QList<QHostAddress> all_addrs = QNetworkInterface::allAddresses();
    for (const QHostAddress &addr: all_addrs) {
        if (addr.protocol() == QAbstractSocket::IPv4Protocol && addr != QHostAddress(QHostAddress::LocalHost)) {
            return addr.toString();
        }
    }
    return {};
}

bool udp_wave_config::loadConfig() {
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
    QJsonObject jsonObj = doc.object();

    // 将根obj的内容，分键读出至对应属性
    readUdpConfig(jsonObj["udp_config"].toObject());
    readChannelConfig(jsonObj["channel_config"].toObject());
    readWaveConfig(jsonObj["wave_config"].toArray());
    readFormatConfig(jsonObj["format_config"].toObject());
    readNoiseConfig(jsonObj["noise_config"].toObject());

    return true;
}


bool udp_wave_config::saveConfig() {
    // 产生要写入的根obj
    QJsonObject jsonObj;
    jsonObj["udp_config"] = writeUdpConfig();
    jsonObj["channel_config"] = writeChannelConfig();
    jsonObj["wave_config"] = writeWaveConfig();
    jsonObj["format_config"] = writeFormatConfig();
    jsonObj["noise_config"] = writeNoiseConfig();

    //产生对应的json doc
    QJsonDocument doc(jsonObj);

    //使用QFile将json doc写入文件
    QFile file(configPath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Cannot open config file for writing:" << configPath;
        return false;
    }
    file.write(doc.toJson());
    file.close();
    qDebug() << "write over";
    return true;
}

void udp_wave_config::createDefaultConfig() {
    QString local_ip = getLocalIPAddress();
    udpConfig = {local_ip, 12345, "192.168.0.2", 54321};
    channelConfig.channels = {1, 0, 1, 1, 0, 1};
    wave_config_vec = {
            {13, 9900,  48000, "FSK", 75,  150, 500, 100},
            {14, 10200, 48000, "MSK", 75,  0,   500, 50},
            {14, 10500, 48000, "MSK", 75,  1,   500, 150},
            {14, 10800, 48000, "FSK", 75,  150, 500, 50},
            {14, 11100, 48000, "MSK", 75,  3,   500, 150},
            {14, 11400, 48000, "FSK", 150, 300, 500, 100}
    };
    formatConfig = {16, "Real"};
    noiseConfig = {1.0};
}

void udp_wave_config::readUdpConfig(const QJsonObject &obj) {
    udpConfig.local_ip = obj["local_ip"].toString();
    udpConfig.local_port = obj["local_port"].toInt();
    udpConfig.dest_ip = obj["dest_ip"].toString();
    udpConfig.dest_port = obj["dest_port"].toInt();
}

QJsonObject udp_wave_config::writeUdpConfig() const {
    QJsonObject obj;
    obj["local_ip"] = udpConfig.local_ip;
    obj["local_port"] = udpConfig.local_port;
    obj["dest_ip"] = udpConfig.dest_ip;
    obj["dest_port"] = udpConfig.dest_port;
    return obj;
}

void udp_wave_config::readChannelConfig(const QJsonObject &obj) {
    QJsonArray channelsArray = obj["channels"].toArray();
    channelConfig.channels.clear();
    for (auto channel: channelsArray)
        channelConfig.channels.append(channel.toInt());
}

QJsonObject udp_wave_config::writeChannelConfig() const {
    QJsonObject obj;
    QJsonArray channelsArray;
    for (auto channel: channelConfig.channels)
        channelsArray.append(channel);
    obj["channels"] = channelsArray;
    return obj;
}

void udp_wave_config::readWaveConfig(const QJsonArray &array) {
    wave_config_vec.clear();
    for (auto value: array) {
        QJsonObject waveObj = value.toObject();
        WaveConfig wave;
        wave.avg_power = waveObj["avg_power"].toDouble();
        wave.carrier_freq = waveObj["carrier_freq"].toInt();
        wave.sample_rate = waveObj["sample_rate"].toInt();
        wave.wave_type = waveObj["wave_type"].toString();
        wave.symbol_rate = waveObj["symbol_rate"].toInt();
        wave.wave_param1 = waveObj["wave_param1"].toInt();
        wave.wave_internal = waveObj["wave_internal"].toInt();
        wave.init_delay = waveObj["init_delay"].toInt();
        wave_config_vec.append(wave);
    }
}

QJsonArray udp_wave_config::writeWaveConfig() const {
    QJsonArray array;
    for (const auto &wave: wave_config_vec) {
        QJsonObject waveObj;
        waveObj["avg_power"] = wave.avg_power;
        waveObj["carrier_freq"] = wave.carrier_freq;
        waveObj["sample_rate"] = wave.sample_rate;
        waveObj["wave_type"] = wave.wave_type;
        waveObj["symbol_rate"] = wave.symbol_rate;
        waveObj["wave_param1"] = wave.wave_param1;
        waveObj["wave_internal"] = wave.wave_internal;
        waveObj["init_delay"] = wave.init_delay;
        array.append(waveObj);
    }
    return array;
}

void udp_wave_config::readFormatConfig(const QJsonObject &obj) {
    formatConfig.data_word_length = obj["data_word_length"].toInt();
    formatConfig.data_type = obj["data_type"].toString();
}

QJsonObject udp_wave_config::writeFormatConfig() const {
    QJsonObject obj;
    obj["data_word_length"] = formatConfig.data_word_length;
    obj["data_type"] = formatConfig.data_type;
    return obj;
}

void udp_wave_config::readNoiseConfig(const QJsonObject &obj) {
    noiseConfig.noise_power_allband = obj["noise_power_allband"].toDouble();
}

QJsonObject udp_wave_config::writeNoiseConfig() const {
    QJsonObject obj;
    obj["noise_power_allband"] = noiseConfig.noise_power_allband;
    return obj;
}

void udp_wave_config::quit() {
    quitNow = true;

}





