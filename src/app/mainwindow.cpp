#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // 多线程相关对象
    recv_thread = new QThread();
    vlf_receiver = new VLFUdpReceiver();
    vlf_receiver->moveToThread(recv_thread);
    // 次线程事件循环停止后，自动销毁相关对象
    connect(recv_thread, &QThread::finished, vlf_receiver, &QObject::deleteLater);
    connect(recv_thread, &QThread::finished, recv_thread, &QObject::deleteLater);


    // 开启多线程环境
    recv_thread->start();
    // 注册接收函数，并开始监听数据端口
    QTimer::singleShot(0, vlf_receiver, &VLFAbstractReceiver::startReceiving);

}

MainWindow::~MainWindow()
{
    if (recv_thread) {
        // 将worker_thread的事件循环退出标志（quitNow）设为true
        recv_thread->exit();
        // 阻塞当前线程，等待finish()函数执行完毕，发出finished信号，处理延迟删除事件，清理线程资源
        recv_thread->wait();
    }
    delete ui;
}

