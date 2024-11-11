#ifndef UDP_MODEM_WIDGET_H
#define UDP_MODEM_WIDGET_H

#include <QWidget>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QNetworkInterface>
#include <QList>
#include <QHostAddress>

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
    double wave_param1;
    int wave_internal;
    int init_delay;
};

struct FormatConfig {
    int data_word_length;
    QString data_type;
};

namespace Ui {
class udp_modem_widget;
}

class udp_modem_widget : public QWidget
{
    Q_OBJECT

public:
    explicit udp_modem_widget(QWidget *parent = nullptr);

    int init();
    QString getLocalIPAddress();
    void updateTableWidgetBackground();


    ~udp_modem_widget();

private:
    Ui::udp_modem_widget *ui;
    int num_channel;
    int num_vheader;

    QString configPath;
    UdpConfig udpConfig;
    ChannelConfig channelConfig;
    QVector<WaveConfig> wave_config_vec;
    FormatConfig formatConfig;

    bool loadConfig();
    bool saveConfig();

    // 对属性赋一个默认初值
    void createDefaultConfig();
    // read 属性 from Json obj/array, write 属性 to Json obj/array
    void readUdpConfig(const QJsonObject& obj);
    QJsonObject writeUdpConfig() const;
    void readChannelConfig(const QJsonObject& obj);
    QJsonObject writeChannelConfig() const;
    void readWaveConfig(const QJsonArray& array);
    QJsonArray writeWaveConfig() const;
    void readFormatConfig(const QJsonObject& obj);
    QJsonObject writeFormatConfig() const;
};

#endif // UDP_MODEM_WIDGET_H
