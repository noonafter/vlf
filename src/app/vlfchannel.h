//
// Created by noisemonitor on 2024/12/16.
//

#ifndef VLF_VLFCHANNEL_H
#define VLF_VLFCHANNEL_H

#include <QObject>


class VLFChannel : public QObject {
Q_OBJECT

public:
    explicit VLFChannel(QObject *parent = nullptr);

    void push_status_package(QByteArray &byte_array);

    void push_business_package(QByteArray &byte_array);

    ~VLFChannel();

private:
    bool is_init;
    int open_state;

    uint8_t channel_id;
    uint8_t data_type; // 0:iq, 1:real
    uint16_t save_type; // 0:i16, 1:i32
    uint32_t sample_rate;
    uint32_t freq_lower_edge;
    uint32_t freq_upper_edge;





};


#endif //VLF_VLFCHANNEL_H
