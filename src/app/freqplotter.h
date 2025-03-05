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
    int set_bin_range(int lo, int up);
    int set_bin_lower(int lo);
    int set_bin_upper(int up);
    int set_db_range(int lo, int up);
    int set_db_lower(int lo);
    int set_db_upper(int up);
    void plot_freq(QVector<double> freq_data);
    void plot_freq(QVector<float> freq_data);
    // 后续考虑加上直接float *和size的plot版本

    ~FreqPlotter() override;

private:

    void init_freq_plot();
    void init_time_freq_plot();
template <typename T>
    void plot_freq_impl(QVector<T> freq_data);
    void update_bin_state();

    QWidget *container;
    bool half_range;
    bool shift_range;
    QVBoxLayout *layout;
    QCustomPlot * freq_plot;
    QCustomPlot * time_freq_plot;
    QCPColorScale *color_scale;
    QCPColorMap *color_map;
    int map_xsize;
    int map_ysize;
    double *map_data;
    QCPMarginGroup *group;
    PlotMode m_plot_mode;

    // 频谱图
    int freq_bin_step; // 频谱图bin间隔
    int m_fft_size;

    enum BinState {
        NeedUpdate, // bin边界跟随fft_size变化，且fft_size变化
        UptoDate,   // bin边界跟随fft_size变化，但已更新
        ManualSet   // bin边界手动设置，忽略fft_size变化
    };
    BinState m_bin_state;
    int bin_lower; // 设置的要显示的bin下限
    int bin_upper; // 设置的要显示的bin上限
    int db_lower;
    int db_upper;

    int time_lower;
    int time_upper;

    // TODO：加上waterfall一起调
    //  设置单位？5个单位到底什么意思？只影响计算方式，不影响最终绘图dB，改个label就行
    // freq时间平均，滑动窗
    // 暂停
    // 调节绘图速率，用来降低绘图开销的，数据不进入窗，相当于抽取


};


#endif //VLF_FREQPLOTTER_H
