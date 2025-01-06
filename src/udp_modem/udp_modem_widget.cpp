#include "udp_modem_widget.h"
#include "ui_udp_modem_widget.h"

udp_modem_widget::udp_modem_widget(QWidget *parent) :
        QWidget(parent),
        ui(new Ui::udp_modem_widget) {

    QString file_name = QCoreApplication::applicationDirPath() + "/" + "udp_config.json";
    m_config = new udp_wave_config(file_name);
    bussiness_tx_timer = new QTimer(this);
    status_tx_timer = new QTimer(this);

    ui->setupUi(this);
    ui->label_sample_rate->setStyleSheet("color: red;");
    ui->label_noise_power->setStyleSheet("color: red;");

    QString str = "\\b(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\b";
    ui->lineEdit_local_ip->setReadOnly(true);
    ui->lineEdit_local_port->setValidator(new QIntValidator(0, 65535, this));
    ui->lineEdit_dest_ip->setValidator(new QRegExpValidator(QRegExp(str), this));
    ui->lineEdit_dest_port->setValidator(new QIntValidator(0, 65535, this));

    ui->comboBox_word_len->addItem("int16");
    ui->comboBox_word_len->addItem("int32");
    ui->comboBox_data_iq->addItem("Real");
    ui->comboBox_data_iq->addItem("I&Q");

    ui->doubleSpinBox_avg_power->setSuffix(" dB");
    ui->comboBox_sample_rate->addItem(("96000 Hz"));
    ui->comboBox_sample_rate->addItem(("192000 Hz"));
    ui->comboBox_sample_rate->addItem(("384000 Hz"));
    ui->comboBox_symbol_rate->addItem("75 Hz");
    ui->comboBox_symbol_rate->addItem("150 Hz");
    ui->comboBox_wave_type->addItem("FSK");
    ui->comboBox_wave_type->addItem("MSK");
    ui->checkBox_freqset->setCheckState(Qt::Checked);
    ui->spinBox_waveinternal->setSuffix(" ms");
    ui->spinBox_init_delay->setSuffix(" ms");
    ui->doubleSpinBox_noise_power->setSuffix(" dB");

    // connect udp ip&port
    connect(ui->lineEdit_local_port, &QLineEdit::editingFinished, this,
            &udp_modem_widget::slot_lineEdit_local_port_editingFinished);
    connect(ui->lineEdit_dest_ip, &QLineEdit::editingFinished, this,
            &udp_modem_widget::slot_lineEdit_dest_ip_editingFinished);
    connect(ui->lineEdit_dest_port, &QLineEdit::editingFinished, this,
            &udp_modem_widget::slot_lineEdit_dest_port_editingFinished);

    // connect channel config
    // findChild扫描groupBox，得到channel的数量，同时connect每个checkbox
    num_channel = 0;
    QString cboxName = QString("checkBox_ch%1").arg(num_channel + 1);
    QCheckBox *checkBox = ui->groupBox_channel_config->findChild<QCheckBox *>(cboxName);
    while (checkBox) {
        connect(checkBox, &QCheckBox::stateChanged, this, &udp_modem_widget::slot_checkBox_chx_stateChanged);
        cboxName = QString("checkBox_ch%1").arg(++num_channel + 1);
        checkBox = ui->groupBox_channel_config->findChild<QCheckBox *>(cboxName);
    }
    connect(ui->checkBox_all_channel_on, &QCheckBox::clicked, this,
            &udp_modem_widget::slot_checkBox_all_channel_on_clicked);

    // connect wave config
    connect(ui->tableWidget, &QTableWidget::cellClicked, this, &udp_modem_widget::slot_tableWidget_cell_clicked);
    connect(ui->doubleSpinBox_avg_power, &QDoubleSpinBox::editingFinished, this,
            &udp_modem_widget::slot_doubleSpinBox_avg_power_editingFinished);
    connect(ui->doubleSpinBox_carrier_freq, &QDoubleSpinBox::editingFinished, this,
            &udp_modem_widget::slot_doubleSpinBox_carrier_freq_editingFinished);
    connect(ui->comboBox_sample_rate, QOverload<const QString &>::of(&QComboBox::currentIndexChanged), this,
            &udp_modem_widget::slot_comboBox_sample_rate_currentIndexChanged);
    connect(ui->comboBox_wave_type, QOverload<const QString &>::of(&QComboBox::currentIndexChanged), this,
            &udp_modem_widget::slot_comboBox_wave_type_currentIndexChanged);
    connect(ui->comboBox_symbol_rate, QOverload<const QString &>::of(&QComboBox::currentIndexChanged), this,
            &udp_modem_widget::slot_comboBox_symbol_rate_currentIndexChanged);
    connect(ui->spinBox_waveparam1, &QSpinBox::editingFinished, this,
            &udp_modem_widget::slot_spinBox_waveparam1_editingFinished);
    connect(ui->spinBox_waveinternal, &QSpinBox::editingFinished, this,
            &udp_modem_widget::slot_spinBox_waveinternal_editingFinished);
    connect(ui->spinBox_init_delay, &QSpinBox::editingFinished, this,
            &udp_modem_widget::slot_spinBox_init_delay_editingFinished);
    connect(ui->doubleSpinBox_noise_power, &QDoubleSpinBox::editingFinished, this,
            &udp_modem_widget::slot_doubleSpinBox_noise_power_editingFinished);
    connect(ui->comboBox_word_len, QOverload<const QString &>::of(&QComboBox::currentIndexChanged), this,
            &udp_modem_widget::slot_comboBox_word_len_currentIndexChanged);
    connect(ui->comboBox_data_iq, QOverload<const QString &>::of(&QComboBox::currentIndexChanged), this,
            &udp_modem_widget::slot_comboBox_data_iq_currentIndexChanged);

    // control zone
    // 多线程相关对象
    worker_thread = new QThread();
    sig_worker = new udp_modem_worker();
    sig_worker->moveToThread(worker_thread);
    // 次线程事件循环停止后，自动销毁相关对象
    connect(worker_thread, &QThread::finished, sig_worker, &QObject::deleteLater);
    connect(worker_thread, &QThread::finished, worker_thread, &QObject::deleteLater);

    connect(ui->pushButton_start, &QPushButton::clicked, this,[=](){
        ui->pushButton_start->setEnabled(false);
        ui->pushButton_stop->setEnabled(true);
        m_config->quitNow = false;
        bussiness_tx_timer->start(256); // 256ms : 192 business package
        status_tx_timer->start(300000);
    });
    // 队列连接
//    connect(ui->pushButton_start, &QPushButton::clicked, sig_worker, &udp_modem_worker::udp_tx_business);
    connect(ui->pushButton_stop, &QPushButton::clicked, this, [=](){
        ui->pushButton_stop->setEnabled(false);
        ui->pushButton_start->setEnabled(true);
        m_config->quitNow = true;
        bussiness_tx_timer->stop();
        status_tx_timer->stop();
    });

    connect(bussiness_tx_timer, &QTimer::timeout, sig_worker, &udp_modem_worker::udp_tx_business);
    connect(status_tx_timer, &QTimer::timeout, sig_worker, &udp_modem_worker::udp_tx_status);



    // freq set lock
    connect(ui->checkBox_freqset, &QCheckBox::stateChanged, ui->doubleSpinBox_carrier_freq,
            &FreqSpinBox::set_freq_set_lock);

}

udp_modem_widget::~udp_modem_widget() {

    if(m_config){
        // 通知停止发送
        m_config->quit();
        // 保持配置到json文件
        m_config->saveConfig();
    }
    if (worker_thread) {
        // 将worker_thread的事件循环退出标志（quitNow）设为true
        worker_thread->exit();
        // 阻塞当前线程，等待finish()函数执行完毕，发出finished信号，处理延迟删除事件，清理线程资源
        worker_thread->wait();
    }
    delete m_config;
    delete ui;

}

int udp_modem_widget::init() {

    // 从json文件中加载配置
    m_config->loadConfig();

    // 更新控件（读取配置后，更新一次，不用写成函数）
    // udp ip&port
    ui->lineEdit_local_ip->setText(m_config->udpConfig.local_ip);
    ui->lineEdit_local_port->setText(QString::number(m_config->udpConfig.local_port));
    ui->lineEdit_dest_ip->setText(m_config->udpConfig.dest_ip);
    ui->lineEdit_dest_port->setText(QString::number(m_config->udpConfig.dest_port));

    // channel on/off
    for (int i = 0; i < num_channel; i++) {
        QString cboxName = QString("checkBox_ch%1").arg(i + 1);
        QCheckBox *checkBox = ui->groupBox_channel_config->findChild<QCheckBox *>(cboxName);
        if (checkBox) {
            checkBox->setCheckState(m_config->channelConfig.channels[i] ? Qt::Checked : Qt::Unchecked);
        }
    }

    // wave params，包括tableWidget和wave param区
    num_vheader = 8;
    ui->tableWidget->setRowCount(num_channel);
    ui->tableWidget->setColumnCount(num_vheader);
    QStringList vheaders = {"通道索引", "平均功率/dBm", "载波频率", "采样速率", "波形类型", "符号速率", "波形参数1", "UDP包计数"};
    ui->tableWidget->setHorizontalHeaderLabels(vheaders);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableWidget->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableWidget->verticalHeader()->setVisible(false);
    for (int row = 0; row < num_channel; ++row) {
        ui->tableWidget->setItem(row, 0, new QTableWidgetItem(QString::number(row + 1)));
        ui->tableWidget->setItem(row, 1, new QTableWidgetItem(QString::number(m_config->wave_config_vec[row].avg_power)));
        ui->tableWidget->setItem(row, 2, new QTableWidgetItem(QString::number(m_config->wave_config_vec[row].carrier_freq)));
        ui->tableWidget->setItem(row, 3, new QTableWidgetItem(QString::number(m_config->wave_config_vec[row].sample_rate)));
        ui->tableWidget->setItem(row, 4, new QTableWidgetItem(m_config->wave_config_vec[row].wave_type));
        ui->tableWidget->setItem(row, 5, new QTableWidgetItem(QString::number(m_config->wave_config_vec[row].symbol_rate)));
        ui->tableWidget->setItem(row, 6, new QTableWidgetItem(QString::number(m_config->wave_config_vec[row].wave_param1)));
        ui->tableWidget->setItem(row, 7, new QTableWidgetItem(QString::number(0)));
    }
    updateTableWidgetBackground();
    // 默认选中第一行，第一列
    slot_tableWidget_cell_clicked(0, 0);

    // data format
    if (m_config->formatConfig.data_word_length == 16) {
        ui->comboBox_word_len->setCurrentIndex(0);
    } else {
        ui->comboBox_word_len->setCurrentIndex(1);
    }

    if (m_config->formatConfig.data_type == "Real") {
        ui->comboBox_data_iq->setCurrentIndex(0);
    } else {
        ui->comboBox_data_iq->setCurrentIndex(1);
    }

    // 开启多线程
    sig_worker->setMConfig(m_config);
    worker_thread->start();

    return 0;
}

void udp_modem_widget::updateTableWidgetBackground() {
    for (int row = 0; row < num_channel; ++row) {
        QTableWidgetItem *item = ui->tableWidget->item(row, 0);
        if (!item) {
            continue;
        }

        if (m_config->channelConfig.channels[row]) {
            item->setBackground(QColor(144, 238, 144));
        } else {
            item->setBackground(Qt::gray);
        }
    }
}

void udp_modem_widget::updateTableWidgetRowItems(int row) {
    ui->tableWidget->setItem(row, 0, new QTableWidgetItem(QString::number(row + 1)));
    ui->tableWidget->setItem(row, 1, new QTableWidgetItem(QString::number(m_config->wave_config_vec[row].avg_power)));
    ui->tableWidget->setItem(row, 2, new QTableWidgetItem(QString::number(m_config->wave_config_vec[row].carrier_freq)));
    ui->tableWidget->setItem(row, 3, new QTableWidgetItem(QString::number(m_config->wave_config_vec[row].sample_rate)));
    ui->tableWidget->setItem(row, 4, new QTableWidgetItem(m_config->wave_config_vec[row].wave_type));
    ui->tableWidget->setItem(row, 5, new QTableWidgetItem(QString::number(m_config->wave_config_vec[row].symbol_rate)));
    ui->tableWidget->setItem(row, 6, new QTableWidgetItem(QString::number(m_config->wave_config_vec[row].wave_param1)));
    ui->tableWidget->setItem(row, 7, new QTableWidgetItem(QString::number(0)));
}

void udp_modem_widget::slot_checkBox_chx_stateChanged(int state) {

    QCheckBox *checkBox_cnt = qobject_cast<QCheckBox *>(sender());
    if (checkBox_cnt) {
        QString check_name = checkBox_cnt->objectName();
        int ch_idx = check_name.rightRef(1).toInt() - 1;
        m_config->channelConfig.channels[ch_idx] = state == Qt::Checked ? 1 : 0;
    }
    updateTableWidgetBackground();

    // 将checkBox_all_channel_on设置为clicked触发，而不是stateChanged触发，可以避免这里重复进入，导致bug
    // clicked由用户发起，会改变所有的checkBox的状态； 而这里改变checkBox_all_channel_on的状态，只是用于显示，不会进一步触发其他操作
    int checkedCount = 0;
    for (int i = 0; i < num_channel; ++i) {
        checkedCount += m_config->channelConfig.channels[i];
    }
    if (checkedCount >= num_channel) {
        ui->checkBox_all_channel_on->setCheckState(Qt::Checked);
    } else {
        ui->checkBox_all_channel_on->setCheckState(Qt::Unchecked);
    }

}

void udp_modem_widget::slot_checkBox_all_channel_on_clicked(int state) {

    Qt::CheckState c_state = state ? Qt::Checked : Qt::Unchecked;
    for (int i = 0; i < num_channel; ++i) {
        QCheckBox *checkBox = findChild<QCheckBox *>(QString("checkBox_ch%1").arg(i + 1));
        if (checkBox) {
            checkBox->setCheckState(c_state);
        }
    }

}

void udp_modem_widget::slot_tableWidget_cell_clicked(int row, int col) {

    Q_UNUSED(col);

    current_set_channel = row;
    WaveConfig wavec = m_config->wave_config_vec[row];

    QString gtitle = QString("波形参数配置-通道%1").arg(row + 1);
    ui->groupBox_waveparam->setTitle(gtitle);
    ui->doubleSpinBox_avg_power->setValue(wavec.avg_power);
    ui->doubleSpinBox_carrier_freq->setValue(wavec.carrier_freq / 1000.0);
    ui->comboBox_sample_rate->setCurrentText(QString::number(wavec.sample_rate)+" Hz");

    if (wavec.wave_type == "FSK") {
        ui->comboBox_wave_type->setCurrentIndex(0);
        ui->label_wave_param->setText("频率间隔：");
        ui->spinBox_waveparam1->setSuffix(" Hz");
    } else {
        ui->comboBox_wave_type->setCurrentIndex(1);
        ui->label_wave_param->setText("初始相位：");
        ui->spinBox_waveparam1->setSuffix(" pi/2");
    }

    ui->spinBox_waveparam1->setValue(wavec.wave_param1);
    ui->spinBox_waveinternal->setValue(wavec.wave_internal);
    ui->spinBox_init_delay->setValue(wavec.init_delay);
    ui->doubleSpinBox_noise_power->setValue(m_config->noiseConfig.noise_power_allband);


}

void udp_modem_widget::slot_doubleSpinBox_avg_power_editingFinished() {

    int row = current_set_channel;
    // 更新类属性
    m_config->wave_config_vec[row].avg_power = ui->doubleSpinBox_avg_power->value();
    // 更新界面
    ui->tableWidget->item(row, 1)->setText(QString::number(m_config->wave_config_vec[row].avg_power));

    // 回车后强制失去焦点，给用户一个反馈，同时避免重复触发
    ui->doubleSpinBox_avg_power->blockSignals(true);
    ui->doubleSpinBox_avg_power->clearFocus();
    ui->doubleSpinBox_avg_power->blockSignals(false);
}

void udp_modem_widget::slot_doubleSpinBox_carrier_freq_editingFinished() {

    int row = current_set_channel;
    int prepare_freq = (int) (ui->doubleSpinBox_carrier_freq->value() * 1000);

    for (int i = 0; i < num_channel; ++i) {
        if (i != row && m_config->wave_config_vec[i].carrier_freq == prepare_freq) {
            // 先警告（阻塞），用户点击ok后，再将值改变回来，提升用户体验
            ui->doubleSpinBox_carrier_freq->blockSignals(true);
            QMessageBox::warning(ui->doubleSpinBox_carrier_freq, "警告", "与其他通道频率重复");
            ui->doubleSpinBox_carrier_freq->blockSignals(false);
            ui->doubleSpinBox_carrier_freq->setValue(m_config->wave_config_vec[row].carrier_freq / 1000.0);
            return;
        }
    }

    // 更新类属性
    m_config->wave_config_vec[row].carrier_freq = prepare_freq;
    // 更新界面
    ui->tableWidget->item(row, 2)->setText(QString::number(m_config->wave_config_vec[row].carrier_freq));

    ui->doubleSpinBox_carrier_freq->blockSignals(true);
    ui->doubleSpinBox_carrier_freq->clearFocus();
    ui->doubleSpinBox_carrier_freq->blockSignals(false);

}

void udp_modem_widget::slot_lineEdit_local_port_editingFinished() {

    // 更新类属性
    m_config->udpConfig.local_port = ui->lineEdit_local_port->text().toInt();

    ui->lineEdit_local_port->blockSignals(true);
    ui->lineEdit_local_port->clearFocus();
    ui->lineEdit_local_port->blockSignals(false);
}

void udp_modem_widget::slot_lineEdit_dest_ip_editingFinished() {

    // 更新类属性
    m_config->udpConfig.dest_ip = ui->lineEdit_dest_ip->text();

    ui->lineEdit_dest_ip->blockSignals(true);
    ui->lineEdit_dest_ip->clearFocus();
    ui->lineEdit_dest_ip->blockSignals(false);

}

void udp_modem_widget::slot_lineEdit_dest_port_editingFinished() {

    // 更新类属性
    m_config->udpConfig.dest_port = ui->lineEdit_dest_port->text().toInt();

    ui->lineEdit_dest_port->blockSignals(true);
    ui->lineEdit_dest_port->clearFocus();
    ui->lineEdit_dest_port->blockSignals(false);

}

void udp_modem_widget::slot_comboBox_sample_rate_currentIndexChanged(const QString &text) {

    int row = 0;
    for (auto &wave: m_config->wave_config_vec) {
        // 更新类属性
        wave.sample_rate = text.leftRef(text.length() - 3).toInt();
        // 更新界面
        ui->tableWidget->item(row++, 3)->setText(QString::number(wave.sample_rate));
    }

}

void udp_modem_widget::slot_comboBox_wave_type_currentIndexChanged(const QString &text) {

    WaveConfig *wavec = &m_config->wave_config_vec[current_set_channel];
    // 更新类属性
    wavec->wave_type = text;
    // 更新界面
    ui->tableWidget->item(current_set_channel, 4)->setText(text);

    if (text == "FSK") {
        ui->label_wave_param->setText("频率间隔：");
        ui->spinBox_waveparam1->setSuffix(" Hz");
        // FSK freq_sep默认设置为2*fsy，减少控件间的耦合
//        ui->spinBox_waveparam1->setValue(wavec->wave_param1 = wavec->symbol_rate * 2);
    } else {
        ui->label_wave_param->setText("初始相位：");
        ui->spinBox_waveparam1->setSuffix(" pi/2");
        // MSK 初始相位默认设置为0
//        ui->spinBox_waveparam1->setValue(wavec->wave_param1 = 0);
    }
    ui->tableWidget->item(current_set_channel, 6)->setText(QString::number(wavec->wave_param1));
}

void udp_modem_widget::slot_comboBox_symbol_rate_currentIndexChanged(const QString &text) {

    WaveConfig *wavec = &m_config->wave_config_vec[current_set_channel];
    // 更新类属性
    wavec->symbol_rate = text.leftRef(text.length() - 3).toInt();
    // 更新界面
    ui->tableWidget->item(current_set_channel, 5)->setText(QString::number(wavec->symbol_rate));

}

void udp_modem_widget::slot_spinBox_waveparam1_editingFinished() {

    WaveConfig *wavec = &m_config->wave_config_vec[current_set_channel];
    wavec->wave_param1 = ui->spinBox_waveparam1->value();
    ui->tableWidget->item(current_set_channel, 6)->setText(QString::number(wavec->wave_param1));

    ui->spinBox_waveparam1->blockSignals(true);
    ui->spinBox_waveparam1->clearFocus();
    ui->spinBox_waveparam1->blockSignals(false);
}

void udp_modem_widget::slot_spinBox_init_delay_editingFinished() {

    WaveConfig *wavec = &m_config->wave_config_vec[current_set_channel];
    wavec->init_delay = ui->spinBox_init_delay->value();

    ui->spinBox_init_delay->blockSignals(true);
    ui->spinBox_init_delay->clearFocus();
    ui->spinBox_init_delay->blockSignals(false);
}

void udp_modem_widget::slot_spinBox_waveinternal_editingFinished() {

    WaveConfig *wavec = &m_config->wave_config_vec[current_set_channel];

    wavec->wave_internal = ui->spinBox_waveinternal->value();

    ui->spinBox_waveinternal->blockSignals(true);
    ui->spinBox_waveinternal->clearFocus();
    ui->spinBox_waveinternal->blockSignals(false);

}

void udp_modem_widget::slot_doubleSpinBox_noise_power_editingFinished() {

    m_config->noiseConfig.noise_power_allband = ui->doubleSpinBox_noise_power->value();

    ui->doubleSpinBox_noise_power->blockSignals(true);
    ui->doubleSpinBox_noise_power->clearFocus();
    ui->doubleSpinBox_noise_power->blockSignals(false);

}

void udp_modem_widget::slot_comboBox_word_len_currentIndexChanged(const QString &text) {

    if (text == "int16") {
        m_config->formatConfig.data_word_length = 16;
    } else {
        m_config->formatConfig.data_word_length = 32;
    }

}

void udp_modem_widget::slot_comboBox_data_iq_currentIndexChanged(const QString &text) {

    if (text == "Real") {
        m_config->formatConfig.data_type = "Real";
    } else {
        m_config->formatConfig.data_type = "I&Q";
    }

}








