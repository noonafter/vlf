//
// Created by lc on 2025/2/22.
//

#ifndef VLF_FREQPLOTTER_H
#define VLF_FREQPLOTTER_H

#include <QWidget>
#include "qcustomplot.h"

// 支持两种模式：功率谱图（FreqMode）；瀑布图（TimeFreqMode），
// 支持模式动态切换，两种模式均通过槽函数plot_freq进行绘制
// 支持显示全部/一半频谱数据，默认显示全部数据
// 支持上下半频谱移位，默认不移位
// 支持刷新率动态调整及图像暂停，默认30fps
//
// todo：
// 支持频率轴范围及显示精度动态调整
// 支持频率轴缩放和拖拽
// 支持dBm范围及单位动态调整
// 支持功率谱图频率间隔动态调整
// 支持功率谱图定点频率测量
// 支持功率谱图峰值显示，AB标签
// 支持瀑布图（色图）尺寸及调色板动态调整

class FreqPlotter : public QWidget {
Q_OBJECT

public:
    enum PlotMode { FreqMode, TimeFreqMode};


// 提升为FreqPlotter即可使用
    explicit FreqPlotter(QWidget *parent = nullptr);
    void setPlotMode(PlotMode mode);
    void togglePlotMode();
    int set_bin_range(int lo, int up);
    int set_bin_lower(int lo);
    int set_bin_upper(int up);
    int set_db_range(int lo, int up);
    int set_db_lower(int lo);
    int set_db_upper(int up);
    bool get_half_range() const;
    void set_half_range(bool half);
    bool get_shift_range() const;
    void set_shift_range(bool shift);
    int get_fft_size() const;
    void set_fft_size( int size);
    double get_sample_frequency() const;
    void set_sample_frequency(double fsa);
    void set_plot_paused(bool paused);
    void set_frame_per_second(int fps);
    void plot_freq(const QVector<double>& freq_data);
    void plot_freq(const QVector<float>& freq_data);
    // 后续考虑加上直接float *和size的plot版本，跨线程会有风险

    ~FreqPlotter() override;

private:
    enum BinState {
        NeedUpdate, // bin边界跟随fft_size变化，且fft_size变化
        UptoDate,   // bin边界跟随fft_size变化，但已更新
        ManualSet   // bin边界手动设置，忽略fft_size变化
    };


    class FreqTicker : public QCPAxisTicker {
    public:
        explicit FreqTicker(const FreqPlotter* parent);
    protected:
        // 重写 generate 以捕获当前刻度步长
        void generate(const QCPRange &range, const QLocale &locale, QChar formatChar, int precision, QVector<double> &ticks, QVector<double> *subTicks, QVector<QString> *tickLabels) override;
        // 动态调整小数位数
        QString getTickLabel(double tick, const QLocale &locale, QChar formatChar, int precision) override;
    private:
        const FreqPlotter* m_parent;
        double m_currentStep; // 当前刻度间隔（步长）
    };

    bool half_range;
    bool shift_range;
    QVBoxLayout *layout;
    QCustomPlot * freq_plot;
    QCustomPlot * time_freq_plot;
    QCPColorScale *color_scale;
    QCPColorMap *color_map;
    QSharedPointer<FreqTicker> m_ticker;
    int map_xsize;
    int map_ysize;
    QCPMarginGroup *group;
    PlotMode m_plot_mode;

    // 频谱图
    int freq_bin_step; // 频谱图bin间隔
    int m_fft_size; // 输入数据的大小，可能是fft_size，也可能是fft_size/2
    double m_fsa;
    
    BinState m_bin_state;
    int bin_lower; // 设置的要显示的bin下限
    int bin_upper; // 设置的要显示的bin上限
    int bin_lowest;
    int bin_upmost;
    int db_minimum;
    int db_maximum;
    int db_lower;
    int db_upper;
    int time_lower;
    int time_upper;
    bool plot_paused;
    QElapsedTimer timer_plot;
    qint64 last_plot_time;
    qint64 cnt_plot_time;
    int plot_internal; // ms

    // TODO：加上waterfall一起调
    //  设置单位？5个单位到底什么意思？只影响计算方式，不影响最终绘图dB，改个label就行
    // freq时间平均，滑动窗
    // waterfall加色卡




    void init_freq_plot();
    void init_time_freq_plot();
    template <typename T>
    void plot_freq_impl(const QVector<T>& freq_data);
    void update_bin_state();
    void clamp_xaxis_range(const QCPRange &newRange);
    void get_bin_range_limit(int &left, int &right) const;


};


#endif //VLF_FREQPLOTTER_H
