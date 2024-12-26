//
// Created by noisemonitor on 2024/12/16.
//

#ifndef VLF_VLFCHANNEL_H
#define VLF_VLFCHANNEL_H

#include <QObject>
#include <QDateTime>
#include "VLFReceiverConfig.h"
#include "readerwriterqueue.h"
#include "FileHandler.h"

using namespace moodycamel;

class VLFChannel : public QObject {
Q_OBJECT

public:
    explicit VLFChannel(QObject *parent = nullptr);
    VLFChannel(int idx_ch);
    bool package_enqueue(const QByteArray &package);

    ~VLFChannel();

public slots:
    void slot_device_info_update(VLFDeviceConfig d_config);
    void slot_channel_info_update(VLFChannelConfig ch_config);
    void slot_business_package_enqueued();

private:

    ReaderWriterQueue<QByteArray> m_queue;
    QByteArray last_channel_params;
    uint32_t last_udp_idx;

    // 设备级参数
    VLFDeviceConfig d_info;
    // 通道级参数
    VLFChannelConfig ch_info;


    // 存储相关，第一级缓存用来接收和存储Rawdata,还是按byte存，后面自己进行cast
    // 两个文件夹rawdata和sigdata，结构相同
    // rawdata/ch0/20241224/RAW_CH0_20241224_171550_192K_RI32_BE

    // 缓存用QByteArray，不用C风格的数组，QByteArray的数据，析构时自动释放了
    QByteArray rawdata_buf;
    int recv_count;



    // 时间间隔和时长，参考winradio接收机，startdate，starttime，repeat_datetime


    QDateTime start_datetime;
    QDateTime current_datetime;

    QDateTime last_datetime;

    int repeat_day;
    int repeat_hour;
    int repeat_minute;

    QTime record_time;

    // 关于vlf通道对象中，file的写入逻辑（为了避免频繁打开和关闭文件）
    // 通道对象存在时，需要频繁使用QFile进行写文件
    // 但是写入的文件可能会变化，上一次还在写a文件，下一次可能需要换b文件写
    // 写入的时候，如果文件有内容，则是追加字节，如果没有文件，则需要创建一个文件并写入，也方便后续追加
    QString app_dir;

    FileHandler rawdata_file_hd;
    QFile rawdata_file;
    bool file_path_changed;
    QString rawdata_file_name;
    QString rawdata_file_path;



    void roundSeconds(QDateTime &dateTime);
};


#endif //VLF_VLFCHANNEL_H
