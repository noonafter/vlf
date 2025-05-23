#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QTimer>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ch_thread(CHANNEL_COUNT), vlf_ch(CHANNEL_COUNT)
    , ui(new Ui::MainWindow)
{
    qRegisterMetaType<QSharedPointer<QByteArray>>("QSharedPointer<QByteArray>");
    qRegisterMetaType<VLFDeviceConfig>("VLFDeviceConfig");
    qRegisterMetaType<VLFChannelConfig>("VLFChannelConfig");

    // 获取资源（对象）
    ui->setupUi(this);
    ui->range_slider_db_if->SetRange(-160, 10);
    ui->range_slider_bin_if->SetRange(0,300);


    FreqPlotter *freqPlotter_ddc = new FreqPlotter(512, FreqPlotter::SPECTRUM, FreqPlotter::FULL_SHIFTED);
    freqPlotter_ddc->set_sample_rate(300);
    ui->widget_ddc->setLayout(new QHBoxLayout());
    ui->widget_ddc->layout()->addWidget(freqPlotter_ddc);

    FreqPlotter *freqPlotter_if = new FreqPlotter(8192, FreqPlotter::SPECTRUM, FreqPlotter::HALF_LOWER);
    freqPlotter_if->set_sample_rate(192000);
    ui->widget_if->setLayout(new QHBoxLayout());
    ui->widget_if->layout()->addWidget(freqPlotter_if);


    QString file_name = QCoreApplication::applicationDirPath() + "/" + "receiver_config.json";
    recv_config = new VLFReceiverConfig(file_name);
    // 多线程对象
    recv_thread = new QThread();
    vlf_receiver = new VLFUdpReceiver();
    for (int i = 0; i < CHANNEL_COUNT; ++i) {
        ch_thread[i] = new QThread();
        vlf_ch[i] = new VLFChannel(i);
    }

    connect(vlf_ch[0], &VLFChannel::subch_freq_ddc_ready, freqPlotter_ddc, QOverload<const QVector<float>&>::of(&FreqPlotter::plot_freq));
    connect(ui->pushButton_mode_ddc, &QPushButton::clicked, freqPlotter_ddc, &FreqPlotter::toggle_plot_mode);
    connect(ui->spinBox_bin_lower_ddc, QOverload<int>::of(&QSpinBox::valueChanged), freqPlotter_ddc,
            &FreqPlotter::set_bin_min);
    connect(ui->spinBox_bin_upper_ddc, QOverload<int>::of(&QSpinBox::valueChanged), freqPlotter_ddc,
            &FreqPlotter::set_bin_max);
    connect(ui->spinBox_db_lower_ddc,QOverload<int>::of(&QSpinBox::valueChanged), freqPlotter_ddc,
            &FreqPlotter::set_db_min);
    connect(ui->spinBox_db_upper_ddc,QOverload<int>::of(&QSpinBox::valueChanged), freqPlotter_ddc,
            &FreqPlotter::set_db_max);


    connect(vlf_ch[0], &VLFChannel::subch_freq_if_ready, freqPlotter_if, QOverload<const QVector<float>&>::of(&FreqPlotter::plot_freq));
    connect(ui->pushButton_mode_if, &QPushButton::clicked, freqPlotter_if, &FreqPlotter::toggle_plot_mode);
    connect(ui->range_slider_db_if,&RangeSlider::lowerValueChanged,freqPlotter_if, &FreqPlotter::set_db_min);
    connect(ui->range_slider_db_if,&RangeSlider::upperValueChanged,freqPlotter_if, &FreqPlotter::set_db_max);

    connect(ui->range_slider_bin_if,&RangeSlider::lowerValueChanged,[=](int lo){
        freqPlotter_if->set_bin_min(lo * freqPlotter_if->get_fft_size() / 600);
    });
    connect(ui->range_slider_bin_if,&RangeSlider::upperValueChanged,[=](int up){
        freqPlotter_if->set_bin_max(up * freqPlotter_if->get_fft_size() / 600);
    });

    connect(ui->spinBox_subch, QOverload<int>::of(&QSpinBox::valueChanged),vlf_ch[0],&VLFChannel::set_idx_sub_ch);
    connect(ui->spinBox_subch, QOverload<int>::of(&QSpinBox::valueChanged),this,[=](int idx){
        ui->label_subch->setText(QString::number(0.15*idx+10.05));
    });
    connect(ui->spinBox_avg_if,QOverload<int>::of(&QSpinBox::valueChanged),freqPlotter_if,&FreqPlotter::set_avg_len);
    connect(ui->spinBox_fftsize_if,QOverload<int>::of(&QSpinBox::valueChanged),freqPlotter_if,&FreqPlotter::set_fft_size);

    // 处理业务逻辑，保证线程安全
    // 具体业务逻辑：
    // vlf_receiver移动到recv_thread线程，保证协议数据包接收和分发单独使用一个线程，不会卡住主线程
    // vlf_ch[i]移动到对应线程，保证该通道业务包的接收和处理单独使用一个线程，不会卡住主线程
    // vlf_receiver绑定vlf_ch数组，保证接收协议数据包后，能转发给对应通道，能通知各通道更新参数
    // vlf_receiver绑定recv_config，保证接收机正确进行参数配置和管理

    // 次线程事件循环停止后，自动销毁相关对象
    vlf_receiver->moveToThread(recv_thread);
    for (int i = 0; i < CHANNEL_COUNT; i++) {
        vlf_ch[i]->moveToThread(ch_thread[i]);
        // new VLFChannel[CHANNEL_COUNT]这么写访问和析构会有问题，不要这么写。搞一个指针数组，分开new。
        connect(ch_thread[i], &QThread::finished, vlf_ch[i], &QObject::deleteLater); // 直接连接，发出同时
        connect(ch_thread[i], &QThread::finished, ch_thread[i], &QObject::deleteLater);
    }
    connect(recv_thread, &QThread::finished, vlf_receiver, &QObject::deleteLater);
    connect(recv_thread, &QThread::finished, recv_thread, &QObject::deleteLater);

    vlf_receiver->set_vlf_ch(&vlf_ch);
    // 绑定的同时，将设备参数推到所有通道一次，通道参数推到对应通道一次
    vlf_receiver->set_vlf_config(recv_config);

    // 开启多线程环境，启动业务
    for (int i = 0; i < CHANNEL_COUNT; ++i) {
        ch_thread[i]->start();
    }
    recv_thread->start();
    // 注册接收函数，并开始监听数据端口
    QTimer::singleShot(0, vlf_receiver, &VLFAbstractReceiver::startReceiving);

}

MainWindow::~MainWindow()
{

    // 由于队列在消费者中，先停止生产者线程！一般应该将队列放在主线程中的，这样可以解耦双方，并且退出时更安全，随便先退谁都行，队列最后退出
    if (recv_thread) {
        // 将worker_thread的事件循环退出标志（quitNow）设为true
        recv_thread->exit();
        // 阻塞当前线程，等待finish()函数执行完毕，发出finished信号，处理延迟删除事件，清理线程资源
        recv_thread->wait();
    }

    for(int i = 0; i < CHANNEL_COUNT && ch_thread[i]; i++){
        ch_thread[i]->exit();
        ch_thread[i]->wait();
    }



    delete recv_config;

    delete ui;
}
