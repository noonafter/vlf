//
// Created by noisemonitor on 2024/11/20.
//

#ifndef VLF_UDP_WAVE_CONFIG_H
#define VLF_UDP_WAVE_CONFIG_H
#include <QString>
#include <QVector>
#include <QJsonObject>

// 作为udp和波形参数的配置管理类，独立进行json配置文件的读写，并可以作为参数在不同模块间传递，将界面类和线程工作类进行解耦
// 可以完全对应json配置文件的结构进行设计，方便进行添加和修改
// udp和波形参数作为独立类，可以在内部都参数读写进行控制：
// 例如，如果存在多线程的问题，可以在内部加锁进行读写控制，对外提供线程安全的读写功能，降低外部使用的复杂性。
// 当然，这里只有界面类在的主线程会进行修改，也可以不加读写控制，把所有属性public，简化程序

struct UdpConfig {
    QString local_ip;
    int local_port;
    QString dest_ip;
    int dest_port;
};

struct ChannelConfig {
    QVector<int> channels;
};

struct WaveConfig {
    double avg_power;
    int carrier_freq;
    int sample_rate;
    QString wave_type;
    int symbol_rate;
    int wave_param1;
    int wave_internal;
    int init_delay;
};

struct FormatConfig {
    int data_word_length;
    QString data_type;
};

struct NoiseConfig {
    double noise_power_allband;
};


class udp_wave_config {

public:
    explicit udp_wave_config(QString file_path);
    virtual ~udp_wave_config();

    bool loadConfig();
    bool saveConfig();
    // 对属性赋一个默认初值
    void createDefaultConfig();
    static QString getLocalIPAddress();

    void quit();

    bool quitNow;
    QString configPath;
    UdpConfig udpConfig;
    ChannelConfig channelConfig;
    QVector<WaveConfig> wave_config_vec;
    FormatConfig formatConfig;
    NoiseConfig noiseConfig;

private:
    // read 属性 from Json obj/array, write 属性 to Json obj/array
    void readUdpConfig(const QJsonObject& obj);
    QJsonObject writeUdpConfig() const;
    void readChannelConfig(const QJsonObject& obj);
    QJsonObject writeChannelConfig() const;
    void readWaveConfig(const QJsonArray& array);
    QJsonArray writeWaveConfig() const;
    void readFormatConfig(const QJsonObject& obj);
    QJsonObject writeFormatConfig() const;
    void readNoiseConfig(const QJsonObject &obj);
    QJsonObject writeNoiseConfig() const;
};


#endif //VLF_UDP_WAVE_CONFIG_H
