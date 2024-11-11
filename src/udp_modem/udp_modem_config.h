#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

class udp_modem_config {

public:
    udp_modem_config(const QString& filePath);
    bool loadConfig();
    bool saveConfig();

private:
    QString filePath;

    struct UdpConfig {
        QString local_ip;
        int local_port;
        QString dest_ip;
        int dest_port;
    } udpConfig;

    struct ChannelConfig {
        QVector<int> channels;
    } channelConfig;

    struct WaveConfig {
        double avg_power;
        int carrier_freq;
        int sample_rate;
        QString wave_type;
        int symbol_rate;
        double wave_param1;
        int wave_internal;
        int init_delay;
    };

    QVector<WaveConfig> waveConfig;

    struct FormatConfig {
        int data_word_length;
        QString data_type;
    } formatConfig;

    void createDefaultConfig();
    void readUdpConfig(const QJsonObject& obj);
    QJsonObject writeUdpConfig() const;
    void readChannelConfig(const QJsonObject& obj);
    QJsonObject writeChannelConfig() const;
    void readWaveConfig(const QJsonArray& array);
    QJsonArray writeWaveConfig() const;
    void readFormatConfig(const QJsonObject& obj);
    QJsonObject writeFormatConfig() const;

};
