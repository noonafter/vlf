#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QTimer>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ch_thread(CHANNEL_COUNT), vlf_ch(CHANNEL_COUNT)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // 多线程相关对象
    recv_thread = new QThread();
    vlf_receiver = new VLFUdpReceiver();

    for (int i = 0; i < CHANNEL_COUNT; ++i) {
        ch_thread[i] = new QThread();
        vlf_ch[i] = new VLFChannel();
    }

    // 次线程事件循环停止后，自动销毁相关对象
    vlf_receiver->moveToThread(recv_thread);
    for (int i = 0; i < CHANNEL_COUNT; i++) {
        vlf_ch[i]->moveToThread(ch_thread[i]);
        // new VLFChannel[CHANNEL_COUNT]这么写访问和析构会有问题，不要这么写。搞一个指针数组，分开new。
        connect(ch_thread[i], &QThread::finished, vlf_ch[i], &QObject::deleteLater);
        connect(ch_thread[i], &QThread::finished, ch_thread[i], &QObject::deleteLater);
    }

    connect(recv_thread, &QThread::finished, vlf_receiver, &QObject::deleteLater);
    connect(recv_thread, &QThread::finished, recv_thread, &QObject::deleteLater);

    // 开启多线程环境
    for (int i = 0; i < CHANNEL_COUNT; ++i) {
        ch_thread[i]->start();
    }

    recv_thread->start();
    // 注册接收函数，并开始监听数据端口
    QTimer::singleShot(0, vlf_receiver, &VLFAbstractReceiver::startReceiving);

}

MainWindow::~MainWindow()
{
    for(int i = 0; i < CHANNEL_COUNT && ch_thread[i]; i++){
        ch_thread[i]->exit();
        ch_thread[i]->wait();
    }

    if (recv_thread) {
        // 将worker_thread的事件循环退出标志（quitNow）设为true
        recv_thread->exit();
        // 阻塞当前线程，等待finish()函数执行完毕，发出finished信号，处理延迟删除事件，清理线程资源
        recv_thread->wait();
    }

    delete ui;
}

