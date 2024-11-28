#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // control zone
    // 多线程相关对象
    worker_recv_thread = new QThread();
    rx_worker = new recv_worker();
    rx_worker->moveToThread(worker_recv_thread);
    // 次线程事件循环停止后，自动销毁相关对象
    connect(worker_recv_thread, &QThread::finished, rx_worker, &QObject::deleteLater);
    connect(worker_recv_thread, &QThread::finished, worker_recv_thread, &QObject::deleteLater);

    worker_recv_thread->start();
}

MainWindow::~MainWindow()
{
    if (worker_recv_thread) {
        // 将worker_thread的事件循环退出标志（quitNow）设为true
        worker_recv_thread->exit();
        // 阻塞当前线程，等待finish()函数执行完毕，发出finished信号，处理延迟删除事件，清理线程资源
        worker_recv_thread->wait();
    }
    delete ui;
}

