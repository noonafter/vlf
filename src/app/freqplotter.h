//
// Created by lc on 2025/2/22.
//

#ifndef VLF_FREQPLOTTER_H
#define VLF_FREQPLOTTER_H

#include <QWidget>
#include "qcustomplot.h"

// 默认是实数信号输入，实数信号默认只画一半fft
// 带dbm补偿的功能，默认为关闭状态

class FreqPlotter : public QWidget {
Q_OBJECT

public:
    enum PlotMode { FreqMode, TimeFreqMode};


// 绑定一个widget使用，作为container，同时作为parent。因此容器无法切换
    explicit FreqPlotter(QWidget *parent = nullptr, bool isComplex = false);
    void setPlotMode(PlotMode mode);
    void togglePlotMode();

    void plot_freq(QVector<double> freq_data);
    void plot_freq(QVector<float> freq_data);

    ~FreqPlotter() override;

private:

    void init_freq_plot();
    void init_time_freq_plot();
    void update_bin_state();

    QWidget *container;
    bool half_range;
    bool shift_range;
    QVBoxLayout *layout;
    QCustomPlot * freq_plot;
    QCustomPlot * time_freq_plot;
    PlotMode m_plot_mode;

    // 频谱图
    int x_step; // 绘图间隔
    int m_fft_size;

    enum BinState {
        NeedUpdate, // bin边界跟随fft_size变化，且fft_size变化
        UptoDate,   // bin边界跟随fft_size变化，但已更新
        ManualSet   // bin边界手动设置，忽略fft_size变化
    };
    BinState m_bin_state;
    int bin_lower; // 设置的要显示的bin下限
    int bin_upper; // 设置的要显示的bin上限


};


#endif //VLF_FREQPLOTTER_H
