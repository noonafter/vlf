#include "udp_modem_widget.h"
#include "ui_udp_modem_widget.h"

udp_modem_widget::udp_modem_widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::udp_modem_widget)
{

    configPath = QCoreApplication::applicationDirPath() + "/"+"udp_config.json";
    ui->setupUi(this);

    ui->lineEdit_local_ip->setReadOnly(true);
    ui->comboBox_word_len->addItem("int16");
    ui->comboBox_word_len->addItem("int32");
    ui->comboBox_data_iq->addItem("Real");
    ui->comboBox_data_iq->addItem("I&Q");

    // findChild扫描groupBox，得到channel的数量，同时connect每个checkbox
    num_channel = 0;
    QString cboxName = QString("checkBox_ch%1").arg(num_channel + 1);
    QCheckBox *checkBox =  ui->groupBox_channel_config->findChild<QCheckBox *>(cboxName);
    while(checkBox) {
        connect(checkBox, &QCheckBox::stateChanged, this, &udp_modem_widget::on_checkBox_chx_stateChanged);
        cboxName = QString("checkBox_ch%1").arg(++num_channel + 1);
        checkBox = ui->groupBox_channel_config->findChild<QCheckBox *>(cboxName);
    }
    connect(ui->checkBox_all_channel_on, &QCheckBox::clicked, this, &udp_modem_widget::on_checkBox_all_channel_on_clicked);

    // freq set lock with
    ui->checkBox_freqset->setCheckState(Qt::Checked);
    connect(ui->checkBox_freqset, &QCheckBox::stateChanged, ui->doubleSpinBox_carrier_freq, &FreqSpinBox::set_freq_set_lock);

}

udp_modem_widget::~udp_modem_widget()
{
    delete ui;
}

int udp_modem_widget::init() {

    // 从json文件中加载配置
    loadConfig();

    // 显示对应配置（读取配置后，刷新一次，不用写成函数）
    // udp ip&port
    ui->lineEdit_local_ip->setText(udpConfig.local_ip);
    ui->lineEdit_local_port->setText(QString::number(udpConfig.local_port));
    ui->lineEdit_dest_ip->setText(udpConfig.dest_ip);
    ui->lineEdit_dest_port->setText(QString::number(udpConfig.dest_port));

    // channel on/off
    for (int i = 0; i < num_channel; i++) {
        QString cboxName = QString("checkBox_ch%1").arg(i + 1);
        QCheckBox *checkBox = ui->groupBox_channel_config->findChild<QCheckBox *>(cboxName);
        if (checkBox) {
            checkBox->setCheckState(channelConfig.channels[i] ? Qt::Checked : Qt::Unchecked);
        }
    }

    // wave params，包括tableWidget和wave param区
    num_vheader = 8;
    ui->tableWidget->setRowCount(num_channel);
    ui->tableWidget->setColumnCount(num_vheader);
    QStringList vheaders = {"通道索引","平均功率/dBm","载波频率","采样速率","波形类型","符号速率","波形参数1","UDP包计数"};
    ui->tableWidget->setHorizontalHeaderLabels(vheaders);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableWidget->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableWidget->verticalHeader()->setVisible(false);
    for(int row = 0; row < num_channel; ++row){
        ui->tableWidget->setItem(row,0,new QTableWidgetItem(QString::number(row+1)));
        ui->tableWidget->setItem(row,1,new QTableWidgetItem(QString::number(wave_config_vec[row].avg_power)));
        ui->tableWidget->setItem(row,2,new QTableWidgetItem(QString::number(wave_config_vec[row].carrier_freq)));
        ui->tableWidget->setItem(row,3,new QTableWidgetItem(QString::number(wave_config_vec[row].sample_rate)));
        ui->tableWidget->setItem(row,4,new QTableWidgetItem(wave_config_vec[row].wave_type));
        ui->tableWidget->setItem(row,5,new QTableWidgetItem(QString::number(wave_config_vec[row].symbol_rate)));
        ui->tableWidget->setItem(row,6,new QTableWidgetItem(QString::number(wave_config_vec[row].wave_param1)));
        ui->tableWidget->setItem(row,7,new QTableWidgetItem(QString::number(0)));
    }
    updateTableWidgetBackground();

    // data format
    if(formatConfig.data_word_length == 16) {
        ui->comboBox_word_len->setCurrentIndex(0);
    } else{
        ui->comboBox_word_len->setCurrentIndex(1);
    }

    if(formatConfig.data_type == "real") {
        ui->comboBox_data_iq->setCurrentIndex(0);
    } else{
        ui->comboBox_data_iq->setCurrentIndex(1);
    }


    return 0;
}

QString udp_modem_widget::getLocalIPAddress() {

    QList<QHostAddress> all_addrs = QNetworkInterface::allAddresses();
    for(const QHostAddress &addr : all_addrs)
    {
        if(addr.protocol() == QAbstractSocket::IPv4Protocol && addr != QHostAddress(QHostAddress::LocalHost)){
            return addr.toString();
        }
    }
    return QString();
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
    QString local_ip = getLocalIPAddress();
    udpConfig = {local_ip, 12345, "192.168.0.2", 54321};
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

void udp_modem_widget::updateTableWidgetBackground() {
    for(int row = 0; row < num_channel; ++row){
        QTableWidgetItem *item = ui->tableWidget->item(row,0);
        if(!item){
            continue;
        }

        if(channelConfig.channels[row]){
            item->setBackground(QColor(144,238,144));
        } else {
            item->setBackground(Qt::gray);
        }
    }
}

void udp_modem_widget::on_checkBox_chx_stateChanged(int state) {

    QCheckBox *checkBox_cnt = qobject_cast<QCheckBox *>(sender());
    if(checkBox_cnt) {
        QString check_name = checkBox_cnt->objectName();
        int ch_idx = check_name.rightRef(1).toInt() - 1;
        channelConfig.channels[ch_idx] =  state == Qt::Checked ? 1 : 0;
    }
    updateTableWidgetBackground();

    // 将checkBox_all_channel_on设置为clicked触发，而不是stateChanged触发，可以避免这里重复进入，导致bug
    // clicked由用户发起，会改变所有的checkBox的状态； 而这里改变checkBox_all_channel_on的状态，只是用于显示，不会进一步触发其他操作
    int checkedCount = 0;
    for(int i = 0; i < num_channel; ++i){
        checkedCount += channelConfig.channels[i];
    }
    if(checkedCount >= num_channel){
        ui->checkBox_all_channel_on->setCheckState(Qt::Checked);
    } else {
        ui->checkBox_all_channel_on->setCheckState(Qt::Unchecked);
    }

}

void udp_modem_widget::on_checkBox_all_channel_on_clicked(int state) {

    Qt::CheckState c_state = state ? Qt::Checked : Qt::Unchecked;
    for(int i = 0; i < num_channel; ++i){
        QCheckBox *checkBox = findChild<QCheckBox *>(QString("checkBox_ch%1").arg(i+1));
        if(checkBox){
            checkBox->setCheckState(c_state);
        }
    }

}


