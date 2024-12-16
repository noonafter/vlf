//
// Created by noisemonitor on 2024/12/16.
//

#ifndef VLF_VLFABSTRACTRECEIVER_H
#define VLF_VLFABSTRACTRECEIVER_H

#include <QObject>



class VLFAbstractReceiver : public QObject {
Q_OBJECT

public:
    explicit VLFAbstractReceiver(QObject *parent = nullptr);

    // 注册包接收函数，开始监听数据端口
    virtual int startReceiving() = 0;

    // 包接收函数，获取包数据后，统一调用process_package进行包处理（得到包数据后，统一成QByteArray，包处理逻辑是相同的）
    virtual void slot_receiver_readyRead() = 0;

    // 包处理函数，判断是status包还是business包，并推送至对应ch对象
    void process_package(const QByteArray &byte_array);

    ~VLFAbstractReceiver() override;

private:

};


#endif //VLF_VLFABSTRACTRECEIVER_H
