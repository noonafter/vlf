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
#include "fdacoefs.h"



// 256个业务包数据，如果sample是rint32，可以存65536个sample
#define RAWDATA_BUF_SIZE (256*1024) // 262144
#define NUM_CHLZB (5)
#define NUM_CH_SUB (334)
#define MAX_FFTSIZE_SUBCH (512)

VLFChannel::VLFChannel(QObject *parent) : QObject(parent), rawdata_writer(RAWDATA_BUF_SIZE), chlzb_inbuf(NUM_CHLZB),
                                          chlzb(NUM_CHLZB), fft_inbuf(NUM_CH_SUB), fft_outbuf(NUM_CH_SUB) {
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
    record_time = QTime(23, 59, 59, 999);
    rawdata_file_name = "";

    num_ch_chlza = 16;
    dec_factor_chlza = 8;
    //    fir lowpass kaiser 192000 6000 6900 42
    chlza_inbuf = cbuffercf_create_max(256, dec_factor_chlza);
    chlza = firpfbch2_crcf_create(LIQUID_ANALYZER, num_ch_chlza, hh0_len / num_ch_chlza / 2, hh0);

    num_ch_chlzb = 160;
    dec_factor_chlzb = 80;
    //    fir lowpass 24000 138 150 45
    for (int i = 0; i < NUM_CHLZB; i++) {
        chlzb_inbuf[i] = cbuffercf_create_max(dec_factor_chlzb * 2, dec_factor_chlzb);
        chlzb[i] = firpfbch2_crcf_create(LIQUID_ANALYZER, num_ch_chlzb, hh1_len / num_ch_chlzb / 2, hh1);
    }
    for (int i = 0; i < NUM_CH_SUB; i++) {
        fft_inbuf[i] = cbuffercf_create(MAX_FFTSIZE_SUBCH);
    }
}

VLFChannel::VLFChannel(int idx) : rawdata_writer(RAWDATA_BUF_SIZE), chlzb_inbuf(NUM_CHLZB), chlzb(NUM_CHLZB),
                                  fft_inbuf(NUM_CH_SUB), fft_outbuf(NUM_CH_SUB) {
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
    record_time = QTime(23, 59, 59, 999);
    rawdata_file_name = "";

    num_ch_chlza = 16;
    dec_factor_chlza = 8;
//    fir lowpass kaiser 192000 6000 6900 42
    chlza_inbuf = cbuffercf_create_max(512, dec_factor_chlza);
    chlza = firpfbch2_crcf_create(LIQUID_ANALYZER, num_ch_chlza, hh0_len / num_ch_chlza / 2, hh0);

    num_ch_chlzb = 160;
    dec_factor_chlzb = 80;
    //    fir lowpass 24000 138 150 45
    for (int i = 0; i < NUM_CHLZB; i++) {
        chlzb_inbuf[i] = cbuffercf_create_max(dec_factor_chlzb * 4, dec_factor_chlzb);
        chlzb[i] = firpfbch2_crcf_create(LIQUID_ANALYZER, num_ch_chlzb, hh1_len / num_ch_chlzb / 2, hh1);
    }
    for (int i = 0; i < NUM_CH_SUB; i++) {
        fft_inbuf[i] = cbuffercf_create(MAX_FFTSIZE_SUBCH);
    }
    in512 = (fftwf_complex *) fftwf_malloc(sizeof(fftwf_complex) * 512);
    out512 = (fftwf_complex *) fftwf_malloc(sizeof(fftwf_complex) * 512);
    fplan512 = fftwf_plan_dft_1d(512, in512, out512, FFTW_FORWARD, FFTW_MEASURE);

    ch_inbuf = cbufferf_create(MAX_FFTSIZE_CH);
    if_inbuf = (float *) fftwf_malloc(sizeof(float) * MAX_FFTSIZE_CH);
    if_outbuf = (fftwf_complex *) fftwf_malloc(sizeof(fftwf_complex) * (MAX_FFTSIZE_CH / 2 + 1));
    for (int i = 0; i < NUM_FFTPLANS_CH; i++) {
        int length = MIN_FFTSIZE_CH << i; // 1024,2048,4096,...
        if_fplans[i] = fftwf_plan_dft_r2c_1d(length, if_inbuf, if_outbuf, FFTW_MEASURE);
    }

}

VLFChannel::~VLFChannel() {

    fftwf_free(in512);
    fftwf_free(out512);
    fftwf_destroy_plan(fplan512);

    firpfbch2_crcf_destroy(chlza);
    cbuffercf_destroy(chlza_inbuf);
    for (int i = 0; i < NUM_CHLZB; i++) {
        firpfbch2_crcf_destroy(chlzb[i]);
        cbuffercf_destroy(chlzb_inbuf[i]);
    }
    for (int i = 0; i < NUM_CH_SUB; i++) {
        cbuffercf_destroy(fft_inbuf[i]);
    }

    fftwf_free(if_inbuf);
    fftwf_free(if_outbuf);
    for (int i = 0; i < NUM_FFTPLANS_CH; i++) {
        fftwf_destroy_plan(if_fplans[i]);
    }
    cbufferf_destroy(ch_inbuf);

    qDebug() << "id is " << QThread::currentThreadId();
    if (this) {
        qDebug() << "has " << this;
    } else {
        qDebug() << "this null ";
    }

}

bool VLFChannel::package_enqueue(const QByteArray &package) {
    return m_queue.try_enqueue(package);
}

void VLFChannel::slot_device_info_update(VLFDeviceConfig d_config) {

    d_info = d_config;
    QDate cnt_date(d_info.year_month_day / 10000, (d_info.year_month_day / 100) % 100, d_info.year_month_day % 100);
    current_datetime.setDate(cnt_date);
    if(!current_datetime.isValid()){
        qWarning() << "After device_info_update, current_datetime is invalid!";
    }

}

void VLFChannel::slot_channel_info_update(VLFChannelConfig ch_config) {

    ch_info = ch_config;
    last_udp_idx = 0;
    QDataStream out(&last_channel_params, QIODevice::WriteOnly);
    out << ch_info.channel_id
        << ch_info.data_type
        << ch_info.save_type
        << ch_info.sample_rate
        << ch_info.freq_lower_edge
        << ch_info.freq_upper_edge;
}


void VLFChannel::slot_business_package_enqueued() {

    QByteArray package;
    if (!m_queue.try_dequeue(package)) {
        return;
    }
    if (!ch_info.open_state || package.size() < 1076) {
        qDebug() << "open_state false";
        return;
    }
    // 收到业务包后，如果是第一次收到，初始化通道参数；如果不是第一次收到，检查参数与本地是否一致
    // 这个逻辑不对，应该是直接与本地参数进行比较，不一致直接丢弃，不存在下位机改变上位机参数
    if (last_channel_params != package.mid(32, 16)) {
        qDebug() << "last_channel_params false";
        return;
    }

    uint32_t cnt_udp_idx = qToBigEndian(*reinterpret_cast<const uint32_t *>((package.mid(0, 4).constData())));
    // 如果udp包号小于上一次的号且号码邻近，说明这一包晚到了，直接丢掉
    if (cnt_udp_idx <= last_udp_idx && last_udp_idx - cnt_udp_idx < 1 << 10) {
        qDebug() << "cnt_udp_idx <= last_udp_idx false";
        return;
    }
    last_udp_idx = cnt_udp_idx;

    // 第一级Buffer为原始样本的缓存，大小为1024字节的整数倍。用来去掉协议头，并用于Rawdata文件的存储
    // QArrayData包括header+实际数据，没有比较好的方法，只去掉头不进行复制的。当然可以用constData返回const char*，考虑用右值引用来移动数据


    // 处理业务包头

    // 根据包提供的时间，设置current_datetime
    QTime cnt_time(package.at(26), package.at(25), package.at(24), package.mid(20, 4).toUInt());
    current_datetime.setTime(cnt_time);
//    qDebug() << current_datetime;

    // 根据包内信息，获取文件夹路径
    QString ch_id_s = QString::number(ch_info.channel_id);
    QString rawdata_dir_path = app_dir + "/rawdata/ch" + ch_id_s + "/" + current_datetime.toString("yyyyMMdd");

    rawdata_writer.setDir(rawdata_dir_path);
    // 由于有多个线程都要写，这里设置整个文件的路径，可能会有问题，后面写就用绝对路径
//    QDir::setCurrent(rawdata_dir_path);

    // 根据包内信息，获取文件名（绝对路径）
    QString data_type_s = ch_info.data_type ? "R" : "C";
    QString save_type_s = ch_info.save_type ? "I32" : "I16";
    QString fsa_s = QString::number(ch_info.sample_rate);
    QStringList rfn_list;    // RAW_CH0_192000_RI32_BE_20241226_101930
    rfn_list << "RAW" << "CH" + ch_id_s << fsa_s << data_type_s + save_type_s << "BE" << "";
    // 更新last_datetime&文件绝对路径
    if (!last_datetime.isValid()) {
        last_datetime = current_datetime;
        roundSeconds(last_datetime);
        rfn_list.replace(5, last_datetime.toString("yyyyMMdd_hhmmss"));
        rawdata_file_name = rfn_list.join("_");
        qDebug() << "file name inited: " << rawdata_file_name;
    }

    qint64 passed_count = last_datetime.msecsTo(current_datetime) / 10000;
    if (passed_count) {
        QDate previous_date = last_datetime.date();
        last_datetime = last_datetime.addSecs(10*passed_count);
        rfn_list.replace(5, last_datetime.toString("yyyyMMdd_hhmmss"));
        rawdata_file_name = rfn_list.join("_");
        // 如果日期发生变化，则将本地日期加1
        if(last_datetime.date() != previous_date){
            qDebug() << "last datetime: " << last_datetime.date();
            qDebug() << "previous_date: " << previous_date;
            current_datetime.setDate(last_datetime.date());
        }
        qDebug() << "file name changed: " << rawdata_file_name;
    }

//    // 如果文件名改变，重新打开文件
    rawdata_writer.setFile(rawdata_file_name);


    // (current-start) % repeat < record, 则当前包加入缓存，这里其实只输入了current_datetime，这里决定是否写入
    int64_t elapsed_ms = start_datetime.msecsTo(current_datetime);
    int64_t repeat_ms = (repeat_day * 24 * 3600 + repeat_hour * 3600 + repeat_minute * 60) * 1000;
    int64_t record_ms = (record_time.hour() * 3600 + record_time.minute() * 60 + record_time.second()) * 1000;

    // 处理业务包数据
    // 在记录时间内，进行记录。记录和监测分开，rawdata_writer用来记录，rawdata_buf用于后续监测。
    QByteArray pack_data(package.constData() + 52, 1024);
//    if (elapsed_ms % repeat_ms < record_ms) {
    if (0) {
        rawdata_writer.write(pack_data);
    } else { // 超出记录时间，写入已有数据，清空buf
//        qDebug() << "not in record time, record stop";
    }

    // 进行监测，是否需要进行缓存？？？一包1024字节，如果是rint32，就是256个sample，
    // 如果用处理流的这种思路，将重要的数据节点（比如需要缓存，数据格式转换，处理的地方）封装成一个类，
    // 对于这个节点类来说，应该有初始化，决定内部数据存储格式
//    if (rawdata_buf.size() < RAWDATA_BUF_SIZE) {
//        rawdata_buf.append(package.constData() + 52, 1024);
//    } else {
//        qDebug() << "rawdata_buf full, process once";
//
//        // 转成float32，
//
//
//        rawdata_buf.resize(0);
//    }

    // 尝试不缓存，收到1包1024字节，256sample即进行处理
    // 1. 将数据转为complex<float>
    for (int i = 0; i < 256; i++) {
        int32_t tmp = qToBigEndian(*reinterpret_cast<const int32_t *>((pack_data.mid(4 * i, 4).constData())));
        float tmp_float = (float) (tmp * 1.0 / 2147483648.0); // 1<<31
        cbuffercf_push(chlza_inbuf, tmp_float); // float -> complex<float>
        cbufferf_push(ch_inbuf, tmp_float);
    }


    int fft_size_idx = 2;
    if(cbufferf_size(ch_inbuf) >= 65536){
        unsigned int num_read;
        float *r;
        cbufferf_read(ch_inbuf, MIN_FFTSIZE_CH << fft_size_idx, &r, &num_read);
        memmove(if_inbuf, r, num_read * sizeof(float));

        // todo：加窗
        fftwf_execute(if_fplans[fft_size_idx]);

        int half_size = (MIN_FFTSIZE_CH << fft_size_idx)/2+1;
        QVector<float> freq_data(half_size);
        float psd_tmp = 10*log10f(MIN_FFTSIZE_CH << fft_size_idx);
        for (int j = 0; j < half_size; j++) {
            std::complex<float> *tmp = reinterpret_cast<std::complex<float> *>(if_outbuf[j]);
            float energy_cnt = std::norm(*tmp);
            if(energy_cnt <=0){
                freq_data[j] = -180;
            } else{
                freq_data[j] = 10*log10f(energy_cnt) - psd_tmp;
            }
        }
        emit subch_freq_if_ready(freq_data);
        cbufferf_release(ch_inbuf, num_read);
    }

    // 2. 经过第一级channelizer，输入192KHz，输出24KHz
    // 每次输入8个点，输出16个点。共执行32次，每个通道32点
    unsigned int num_read;
    std::complex<float> *chlza_in_tmp;
    std::complex<float> chlza_out_tmp[num_ch_chlza];
    while (cbuffercf_size(chlza_inbuf) >= dec_factor_chlza) {
        // 读取输入，8个sample
        cbuffercf_read(chlza_inbuf, dec_factor_chlza, &chlza_in_tmp, &num_read);

        // 计算结果，16个sample
        firpfbch2_crcf_execute(chlza, chlza_in_tmp, chlza_out_tmp);

        // 结果推送，只处理1,2,3,4,5号通道
        for (int i = 0; i < NUM_CHLZB; i++){
            cbuffercf_push(chlzb_inbuf[i], chlza_out_tmp[1 + i]);
        }

        //更新
        cbuffercf_release(chlza_inbuf, dec_factor_chlza);
    }

    // 3. 经过第二级channelizer（共5个），
    // 每个chlza分num_ch_chlzb=160个通道，每次输入dec_factor_chlz1=80个点（第一级每次只给了32个点），输出num_ch_chlzb=160个点。
    std::complex<float> *chlzb_in_tmp;
    std::complex<float> chlzb_out_tmp[num_ch_chlzb];
    int idx_subch=0;
    // 处理chlzb[0]
    while (cbuffercf_size(chlzb_inbuf[0]) >= dec_factor_chlzb) {
        // 读取输入，80个sample
        cbuffercf_read(chlzb_inbuf[0], dec_factor_chlzb, &chlzb_in_tmp, &num_read);

        // 计算结果，160个sample
        firpfbch2_crcf_execute(chlzb[0], chlzb_in_tmp, chlzb_out_tmp);

        // fft_inbuf[0-12]放入sample[147-159]
        for (int i = 0; i < 13; i++) {
            cbuffercf_push(fft_inbuf[idx_subch++], chlzb_out_tmp[147 + i]);
        }
        // fft_inbuf[13-53]放入sample[0-40]
        for (int i = 0; i < 41; i++) {
            cbuffercf_push(fft_inbuf[idx_subch++], chlzb_out_tmp[i]);
        }
        // 使用完毕
        cbuffercf_release(chlzb_inbuf[0], dec_factor_chlzb);
    }

    // 处理chlzb[1-3]
    for (int idx_chlzb = 1; idx_chlzb < NUM_CHLZB - 1; idx_chlzb++) {
        while (cbuffercf_size(chlzb_inbuf[idx_chlzb]) >= dec_factor_chlzb) {
            // 读取输入，80个sample
            cbuffercf_read(chlzb_inbuf[idx_chlzb], dec_factor_chlzb, &chlzb_in_tmp, &num_read);

            // 计算结果，160个sample
            firpfbch2_crcf_execute(chlzb[idx_chlzb], chlzb_in_tmp, chlzb_out_tmp);

            // fft_inbuf[54-92]放入sample[121-159]
            for (int i = 0; i < 39; i++) {
                cbuffercf_push(fft_inbuf[idx_subch++], chlzb_out_tmp[121 + i]);
            }
            // fft_inbuf[93-133]放入sample[0-40]
            for (int i = 0; i < 41; i++) {
                cbuffercf_push(fft_inbuf[idx_subch++], chlzb_out_tmp[i]);
            }
            // 使用完毕
            cbuffercf_release(chlzb_inbuf[idx_chlzb], dec_factor_chlzb);
        }
    }

    // 处理chlzb[4]
    while (cbuffercf_size(chlzb_inbuf[4]) >= dec_factor_chlzb) {
        // 读取输入，80个sample
        cbuffercf_read(chlzb_inbuf[4], dec_factor_chlzb, &chlzb_in_tmp, &num_read);

        // 计算结果，160个sample
        firpfbch2_crcf_execute(chlzb[4], chlzb_in_tmp, chlzb_out_tmp);

        // fft_inbuf[54-92]放入sample[121-159]
        for (int i = 0; i < 39; i++) {
            cbuffercf_push(fft_inbuf[idx_subch++], chlzb_out_tmp[121 + i]);
        }
        // fft_inbuf[333]放入sample[0]
        cbuffercf_push(fft_inbuf[idx_subch++], chlzb_out_tmp[0]);
        // 使用完毕
        cbuffercf_release(chlzb_inbuf[4], dec_factor_chlzb);
    }

    // 4.如果buf满，执行fft
    int fftsize_subch = 512;
    std::complex<float> *r;
    float psd_tmp = 10*log10f(512);
    for(int i = 0; i<NUM_CH_SUB;i++){
        if(cbuffercf_size(fft_inbuf[i]) >= fftsize_subch){
            cbuffercf_read(fft_inbuf[i], fftsize_subch, &r, &num_read);
            if(i == -1){
                QString wfilename = "D:\\alc\\c\\vlf\\scripts\\data_subch" + QString::number(i);
                QFile wfile(wfilename);
                wfile.open(QIODevice::Append);
                wfile.write(reinterpret_cast<const char *>(r), 512 * sizeof(fftwf_complex));
            }

            memmove(in512, r, 512* sizeof(fftwf_complex));

            fftwf_execute(fplan512);
            QVector<float> freq_data(512);

            if(i == 3){

                for (int j = 0; j < 512; j++) {
                    std::complex<float> *tmp = reinterpret_cast<std::complex<float> *>(out512[j]);
                    float energy_cnt = std::norm(*tmp);
                    if(energy_cnt <=0){
                        freq_data[j] = -180;
                    } else{
                        freq_data[j] = 10*log10f(energy_cnt) - psd_tmp;
                    }
                }
                emit subch_freq_ddc_ready(freq_data);
            }




            cbuffercf_release(fft_inbuf[i], fftsize_subch);
        }
    }



    recv_count++;
    if (!(recv_count % 5000)) {
        qDebug() << "ch" << ch_info.channel_id << "recv_count:" << recv_count;
    }

}

void VLFChannel::roundSeconds(QDateTime &dateTime) {
    int seconds = dateTime.time().second();
    int rseconds = qRound(seconds / 10.0) * 10;
    if (rseconds == 60)
        rseconds = 0;

    dateTime.setTime(QTime(dateTime.time().hour(), dateTime.time().minute(), rseconds));
}




