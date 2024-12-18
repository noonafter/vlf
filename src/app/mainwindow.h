#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QThread>
#include "VLFUdpReceiver.h"
#include "vlfchannel.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

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


};
#endif // MAINWINDOW_H
