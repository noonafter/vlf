#ifndef UDP_MODEM_WIDGET_H
#define UDP_MODEM_WIDGET_H

#include <QWidget>
#include "udp_modem_config.h"

namespace Ui {
class udp_modem_widget;
}

class udp_modem_widget : public QWidget
{
    Q_OBJECT

public:
    explicit udp_modem_widget(QWidget *parent = nullptr);

    int init();
    bool loadConfig();
    bool saveConfig();

    ~udp_modem_widget();

private:
    Ui::udp_modem_widget *ui;
    int num_channel;
    int num_vheader;


    QString configPath;
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
    QVector<WaveConfig> wave_config_vec;

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

#endif // UDP_MODEM_WIDGET_H
