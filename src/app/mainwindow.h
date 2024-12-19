#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QThread>
#include "VLFUdpReceiver.h"
#include "vlfchannel.h"


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

// 所有类都遵循RAII的原则进行设计：通过对象生命周期来管理资源，在构造函数中获取资源，并正确初始化，在析构函数中，释放资源。使用智能指针etc

// MainWindow其实承担了controler的角色：
//1 获取各种资源（对象），包括：主界面，vlf_receiver（包括recv线程），四个通道对象（包括ch线程），receiver配置类
//2 处理业务逻辑，保证主界面（view）与工作类对象（model）之间，以及各个工作类对象之间正确连接和通信，比如
//  2.1 主界面初始化之后，采用信号槽连接参数配置控件和receiver配置类等，响应用户操作
//  2.2 将各个对象moveToThread到对应线程，并使用信号槽与延迟删除等机制，保证线程正常退出的同时，安全销毁线程相关对象
//  2.3 正确关联vlf_receiver对象、四个通道对象和receiver配置类，使用锁、队列连接、阻塞队列机制连接等保证线程间通信和同步，

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    QThread *recv_thread;
    VLFAbstractReceiver *vlf_receiver;
    QVector<QThread*> ch_thread;
    QVector<VLFChannel*> vlf_ch;
    VLFReceiverConfig *recv_config;
};
#endif // MAINWINDOW_H
