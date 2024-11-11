#include "udp_modem_widget.h"
#include "ui_udp_modem_widget.h"

udp_modem_widget::udp_modem_widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::udp_modem_widget)
{
    configPath = QCoreApplication::applicationDirPath() + "/"+"udp_config.json";
    ui->setupUi(this);
}

udp_modem_widget::~udp_modem_widget()
{
    delete ui;
}

int udp_modem_widget::init() {
    num_channel = 6;
    num_vheader = 8;


    loadConfig();

    // udp ip


    // channel on/off

    // wave params

    // data format


    ui->tableWidget->setRowCount(num_channel);
    ui->tableWidget->setColumnCount(num_vheader);
    QStringList vheaders = {"通道索引","平均功率","载波频率","采样速率","波形类型","符号速率","波形参数1","UDP包计数"};
    ui->tableWidget->setHorizontalHeaderLabels(vheaders);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableWidget->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableWidget->verticalHeader()->setVisible(false);

    // freq set lock with
    ui->checkBox_freqset->setCheckState(Qt::Checked);
    connect(ui->checkBox_freqset, &QCheckBox::stateChanged, ui->doubleSpinBox_carrier_freq, &FreqSpinBox::set_freq_set_lock);


    return 0;
}



bool udp_modem_widget::loadConfig() {
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

    return true;
}



bool udp_modem_widget::saveConfig() {
    // 产生要写入的根obj
    QJsonObject jsonObj;
    jsonObj["udp_config"] = writeUdpConfig();
    jsonObj["channel_config"] = writeChannelConfig();
    jsonObj["wave_config"] = writeWaveConfig();
    jsonObj["format_config"] = writeFormatConfig();

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
    return true;
}

void udp_modem_widget::createDefaultConfig() {
    udpConfig = {"192.168.0.1", 12345, "192.168.0.2", 54321};
    channelConfig.channels = {1, 0, 1, 1, 0, 1};
    wave_config_vec = {
            {13,  9900, 48000, "fsk", 75, 150, 500, 100},
            {14, 10200, 48000, "msk", 75, 0.1, 500,  50},
            {14, 10500, 48000, "msk", 75, 0.1, 500, 150},
            {14, 10800, 48000, "fsk", 75, 150, 500,  50},
            {14, 11100, 48000, "msk", 75, 0.1, 500, 150},
            {14, 11400, 48000, "fsk", 75, 150, 500, 100}
    };
    formatConfig = {16, "real"};
}

void udp_modem_widget::readUdpConfig(const QJsonObject& obj) {
    udpConfig.local_ip = obj["local_ip"].toString();
    udpConfig.local_port = obj["local_port"].toInt();
    udpConfig.dest_ip = obj["dest_ip"].toString();
    udpConfig.dest_port = obj["dest_port"].toInt();
}

QJsonObject udp_modem_widget::writeUdpConfig() const {
    QJsonObject obj;
    obj["local_ip"] = udpConfig.local_ip;
    obj["local_port"] = udpConfig.local_port;
    obj["dest_ip"] = udpConfig.dest_ip;
    obj["dest_port"] = udpConfig.dest_port;
    return obj;
}

void udp_modem_widget::readChannelConfig(const QJsonObject& obj) {
    QJsonArray channelsArray = obj["channels"].toArray();
    channelConfig.channels.clear();
    for (auto channel : channelsArray)
        channelConfig.channels.append(channel.toInt());
}

QJsonObject udp_modem_widget::writeChannelConfig() const {
    QJsonObject obj;
    QJsonArray channelsArray;
    for (auto channel : channelConfig.channels)
        channelsArray.append(channel);
    obj["channels"] = channelsArray;
    return obj;
}

void udp_modem_widget::readWaveConfig(const QJsonArray& array) {
    wave_config_vec.clear();
    for (auto value : array) {
        QJsonObject waveObj = value.toObject();
        WaveConfig wave;
        wave.avg_power = waveObj["avg_power"].toDouble();
        wave.carrier_freq = waveObj["carrier_freq"].toInt();
        wave.sample_rate = waveObj["sample_rate"].toInt();
        wave.wave_type = waveObj["wave_type"].toString();
        wave.symbol_rate = waveObj["symbol_rate"].toInt();
        wave.wave_param1 = waveObj["wave_param1"].toDouble();
        wave.wave_internal = waveObj["wave_internal"].toInt();
        wave.init_delay = waveObj["init_delay"].toInt();
        wave_config_vec.append(wave);
    }
}

QJsonArray udp_modem_widget::writeWaveConfig() const {
    QJsonArray array;
    for (const auto& wave : wave_config_vec) {
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

void udp_modem_widget::readFormatConfig(const QJsonObject& obj) {
    formatConfig.data_word_length = obj["data_word_length"].toInt();
    formatConfig.data_type = obj["data_type"].toString();
}

QJsonObject udp_modem_widget::writeFormatConfig() const {
    QJsonObject obj;
    obj["data_word_length"] = formatConfig.data_word_length;
    obj["data_type"] = formatConfig.data_type;
    return obj;
}