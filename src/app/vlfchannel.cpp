//
// Created by noisemonitor on 2024/12/16.
//

// You may need to build the project (run Qt uic code generator) to get "ui_VLFChannel.h" resolved

#include "vlfchannel.h"
#include <QDebug>
#include <QThread>
#include <QtEndian>
#include <QDataStream>
#include <QCoreApplication>
#include <QDir>

// 256个业务包数据，如果sample是rint32，可以存65536个sample
#define RAWDATA_BUF_SIZE (256*1024) // 262144

VLFChannel::VLFChannel(QObject *parent) : QObject(parent), rawdata_writer(RAWDATA_BUF_SIZE) {
    m_queue = ReaderWriterQueue<QByteArray>(512);
    ch_info.open_state = true;
    last_channel_params = QByteArray(16, '\0');
    last_udp_idx = 0;
    recv_count = 0;
    rawdata_buf.reserve(RAWDATA_BUF_SIZE);
    app_dir = QCoreApplication::applicationDirPath();
    current_datetime = QDateTime();
    last_datetime = QDateTime();
    start_datetime = QDateTime::currentDateTime().addDays(-1);
    repeat_day = 1;
    repeat_hour = 0;
    repeat_minute = 0;
    record_time = QTime(23,59,59,999);
    rawdata_file_name = "";
}

VLFChannel::VLFChannel(int idx) : rawdata_writer(RAWDATA_BUF_SIZE) {
    ch_info.channel_id = idx;

    m_queue = ReaderWriterQueue<QByteArray>(512);
    ch_info.open_state = true;
    last_channel_params = QByteArray(16, '\0');
    last_udp_idx = 0;
    recv_count = 0;
    rawdata_buf.reserve(RAWDATA_BUF_SIZE); // capacity = 262,144B
    app_dir = QCoreApplication::applicationDirPath();
    current_datetime = QDateTime();
    last_datetime = QDateTime();
    start_datetime = QDateTime::currentDateTime().addDays(-1);
    repeat_day = 1;
    repeat_hour = 0;
    repeat_minute = 0;
    // 不能写QTime(24,0)
    record_time = QTime(23,59,59,999);
    rawdata_file_name = "";
}

VLFChannel::~VLFChannel() {

    qDebug() << "id is " << QThread::currentThreadId();
    if(this)
    {
        qDebug() << "has " << this;
    }else{
        qDebug() << "this null ";
    }

}

bool VLFChannel::package_enqueue(const QByteArray &package) {
    return m_queue.try_enqueue(package);
}

void VLFChannel::slot_device_info_update(VLFDeviceConfig d_config) {

    d_info = d_config;

}

void VLFChannel::slot_channel_info_update(VLFChannelConfig ch_config) {

    ch_info = ch_config;
    last_udp_idx = 0;
    QDataStream out(&last_channel_params,QIODevice::WriteOnly);
    out << ch_info.channel_id
        << ch_info.data_type
        << ch_info.save_type
        << ch_info.sample_rate
        << ch_info.freq_lower_edge
        << ch_info.freq_upper_edge;
}



void VLFChannel::slot_business_package_enqueued() {

    QByteArray package;
    if(!m_queue.try_dequeue(package)){
        return;
    }

    if (!ch_info.open_state || package.size() < 1076) {
        return;
    }

    // 收到业务包后，如果是第一次收到，初始化通道参数；如果不是第一次收到，检查参数与本地是否一致
    // 这个逻辑不对，应该是直接与本地参数进行比较，不一致直接丢弃，不存在下位机改变上位机参数
    if(last_channel_params != package.mid(32,16)){
        return;
    }

    uint32_t cnt_udp_idx = qToBigEndian(*reinterpret_cast<const uint32_t *>((package.mid(0, 4).constData())));
    // 如果udp包号小于上一次的号且号码邻近，说明这一包晚到了，直接丢掉
    if (cnt_udp_idx <= last_udp_idx && last_udp_idx - cnt_udp_idx < 1<<20) {
        return;
    }
    last_udp_idx = cnt_udp_idx;

    // 第一级Buffer为原始样本的缓存，大小为1024字节的整数倍。用来去掉协议头，并用于Rawdata文件的存储
    // QArrayData包括header+实际数据，没有比较好的方法，只去掉头不进行复制的。当然可以用constData返回const char*，考虑用右值引用来移动数据


    // 处理业务包头

    // 根据包提供的时间，设置current_datetime
    QDate cnt_date(d_info.year_month_day/10000,(d_info.year_month_day/100)%100,d_info.year_month_day%100);
    QTime cnt_time(package.at(26), package.at(25), package.at(24), package.mid(20,4).toUInt());
    current_datetime.setDate(cnt_date);
    current_datetime.setTime(cnt_time);

    // 根据包内信息，获取文件夹路径
    QString ch_id_s = QString::number(ch_info.channel_id);
    QString rawdata_dir_path = app_dir + "/rawdata/ch" + ch_id_s + "/" + cnt_date.toString("yyyyMMdd");

    rawdata_writer.setDir(rawdata_dir_path);
    // 由于有多个线程都要写，这里设置整个文件的路径，可能会有问题，后面写就用绝对路径
//    QDir::setCurrent(rawdata_dir_path);

    // 根据包内信息，获取文件名（绝对路径）
    QString data_type_s = ch_info.data_type ? "R" : "C";
    QString save_type_s = ch_info.save_type ? "I32" : "I16";
    QString fsa_s = QString::number(ch_info.sample_rate);
    QStringList rfn_list;    // RAW_CH0_192000_RI32_BE_20241226_101930
    rfn_list << "RAW" << "CH"+ch_id_s << fsa_s << data_type_s + save_type_s << "BE" << "";
    // 更新last_datetime&文件绝对路径
    if (!last_datetime.isValid()) {
        last_datetime = current_datetime;
        roundSeconds(last_datetime);
        rfn_list.replace(5, last_datetime.toString("yyyyMMdd_hhmmss"));
        rawdata_file_name = rfn_list.join("_");
    }

    if (last_datetime.msecsTo(current_datetime) > (10000 - 50)) {
        last_datetime = last_datetime.addSecs(10);
        rfn_list.replace(5, last_datetime.toString("yyyyMMdd_hhmmss"));
        rawdata_file_name = rfn_list.join("_");
    }

//    // 如果文件名改变，重新打开文件
    rawdata_writer.setFile(rawdata_file_name);


    // (current-start) % repeat < record, 则当前包加入缓存，这里其实只输入了current_datetime，这里决定是否写入
    int64_t elapsed_ms = start_datetime.msecsTo(current_datetime);
    int64_t repeat_ms = (repeat_day * 24 * 3600 + repeat_hour * 3600 + repeat_minute * 60) * 1000;
    int64_t record_ms = (record_time.hour() * 3600 + record_time.minute() * 60 + record_time.second()) * 1000;

    // 处理业务包数据
    // 在记录时间内，进行记录。记录和监测分开，rawdata_writer用来记录，rawdata_buf用于后续监测。
    if (elapsed_ms % repeat_ms < record_ms) {
        rawdata_writer.write(package.constData() + 52, 1024);
    }else{ // 超出记录时间，写入已有数据，清空buf
        rawdata_writer.flushBuffer();
        qDebug() << "exceed record time, record stop";
    }

    // 进行监测
    if (rawdata_buf.size() < RAWDATA_BUF_SIZE) {
        rawdata_buf.append(package.constData() + 52, 1024);
    } else {
        qDebug() << "rawdata_buf full, process once";


        rawdata_buf.resize(0);
    }


    recv_count++;
    if(!(recv_count%5000)){
        qDebug() << "recv_count:" << recv_count;
    }

}

void VLFChannel::roundSeconds(QDateTime &dateTime) {
    int seconds = dateTime.time().second();
    int rseconds = qRound(seconds / 10.0) * 10;
    if (rseconds == 60)
        rseconds = 0;

    dateTime.setTime(QTime(dateTime.time().hour(),dateTime.time().minute(),rseconds));
}




