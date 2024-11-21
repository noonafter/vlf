#ifndef UDP_MODEM_WIDGET_H
#define UDP_MODEM_WIDGET_H

#include <QWidget>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

#include <QList>
#include <QHostAddress>
#include <QMessageBox>
#include <QThread>
#include "udp_modem_worker.h"
#include "udp_wave_config.h"


namespace Ui {
class udp_modem_widget;
}

class udp_modem_widget : public QWidget
{
    Q_OBJECT

public:
    explicit udp_modem_widget(QWidget *parent = nullptr);
    ~udp_modem_widget();

    int init();

public slots:
    // 如果写为on_<obj_name>_<signal>, ui_头文件会进行自动连接一次，但有的可能识别不了，no matching signal
    // 最好改为slot_<obj_name>_<signal>，手动connect
    void slot_checkBox_chx_stateChanged(int state);
    void slot_checkBox_all_channel_on_clicked(int state);
    void slot_tableWidget_cell_clicked(int row, int col);
    void slot_doubleSpinBox_avg_power_editingFinished();
    void slot_doubleSpinBox_carrier_freq_editingFinished();
    void slot_lineEdit_local_port_editingFinished();
    void slot_lineEdit_dest_ip_editingFinished();
    void slot_lineEdit_dest_port_editingFinished();
    void slot_comboBox_sample_rate_currentIndexChanged(const QString &text);
    void slot_comboBox_wave_type_currentIndexChanged(const QString &text);
    void slot_comboBox_symbol_rate_currentIndexChanged(const QString &text);
    void slot_spinBox_waveparam1_editingFinished();
    void slot_spinBox_init_delay_editingFinished();
    void slot_spinBox_waveinternal_editingFinished();
    void slot_doubleSpinBox_noise_power_editingFinished();
    void slot_comboBox_word_len_currentIndexChanged(const QString &text);
    void slot_comboBox_data_iq_currentIndexChanged(const QString &text);

private:
    Ui::udp_modem_widget *ui;
    int num_channel;
    int num_vheader;
    int current_set_channel;

    // multi-thread&signal generate
    QThread *worker_thread;
    udp_modem_worker *sig_worker;
    udp_wave_config *m_config;

    // 根据类属性更新控件
    void updateTableWidgetBackground();
    void updateTableWidgetRowItems(int row);


};

#endif // UDP_MODEM_WIDGET_H
