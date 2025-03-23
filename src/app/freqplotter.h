//
// Created by lc on 2025/2/22.
//

#ifndef VLF_FREQPLOTTER_H
#define VLF_FREQPLOTTER_H

#if defined(__cplusplus) && __cplusplus >= 201703L
#define CPP17_OR_LATER 1
#define IF_CONSTEXPR(...) if constexpr(__VA_ARGS__)
#else
#define CPP17_OR_LATER 0
#define IF_CONSTEXPR(...) if (__VA_ARGS__)
#endif

#include <QWidget>
#include "qcustomplot.h"

// 支持两种绘图模式：功率谱图；瀑布图，并支持模式动态切换。
// 支持4种FFT显示模式：全部FFT点顺序，全部FFT点移位，下半FFT，上半FFT。
// 支持刷新率动态调整及图像暂停，默认30fps
// 支持频率轴显示范围动态调整，显示精度自适应调整
// 支持频率轴拖拽与缩放
// 支持功率谱图定点频率测量(双击)
// 支持功率谱图时间平均

// todo：
// 支持频率轴采样率及单位动态调整，后续对set_fsa函数进行更改，加上默认单位，
// 频率轴支持中心频率调整
// 支持dB范围及单位动态调整
// 支持功率谱图频率轴绘图间隔动态调整，默认间隔1

// 支持功率谱图峰值显示，AB标签
// 支持瀑布图（色图）尺寸及调色板动态调整
// 模式切换后，瀑布图任然一直在画，加一个mapinited开关，开始时关闭不画map，一旦切换到了waterfall就打开
// 画图plot_freq_impl中校验可能要改，根据bin_display校验，而不是count

class QMarkLine;

class FreqPlotter : public QWidget {
Q_OBJECT

public:
    enum PlotMode {
        SPECTRUM, // 绘制功率谱图，默认情况
        WATERFALL // 绘制瀑布图
    };

    enum FFTDisplayMode {
        FULL_SEQUENTIAL, // 展示整个FFT点数，顺序展示，默认情况
        FULL_SHIFTED,    // 展示整个FFT点数，上下频谱移位，常用于复数信号。
        HALF_LOWER,      // 只展示下半部分FFT点数，常用于实数信号
        HALF_UPPER       // 只展示上半部分FFT点数，较少使用
    };

// 提升为FreqPlotter即可使用
    explicit FreqPlotter(QWidget *parent = nullptr);

    explicit FreqPlotter(int fft_size, PlotMode plot_mode = SPECTRUM, FFTDisplayMode fft_mode = FULL_SEQUENTIAL,
                         QWidget *parent = nullptr);

    ~FreqPlotter() override;

    int get_fft_size() const;
    void set_fft_size(int size);
    double get_sample_rate() const;
    void set_sample_rate(double rate);
    FFTDisplayMode get_fft_display_mode() const;
    void set_fft_display_mode(FFTDisplayMode mode);
    PlotMode get_plot_mode() const;
    void set_plot_mode(PlotMode mode);

    int set_bin_range(int min, int max);
    int set_bin_min(int min);
    int set_bin_max(int max);
    int set_db_range(int min, int max);
    int set_db_min(int min);
    int set_db_max(int max);
    void set_plot_paused(bool paused);
    void set_frame_per_second(int fps);
    void set_palette(int idx);
    void set_avg_len(int len);
    void toggle_plot_mode();
    void plot_freq(const QVector<double>& freq_data);
    void plot_freq(const QVector<float>& freq_data);
    // 后续考虑加上直接float *和size的plot版本，跨线程会有风险



private:

    class FreqTicker : public QCPAxisTicker {
    public:
        explicit FreqTicker(const FreqPlotter* parent);
    protected:
        // 重写TickStep
        double getTickStep(const QCPRange &range) override;
        // 动态调整小数位数
        QString getTickLabel(double tick, const QLocale &locale, QChar formatChar, int precision) override;
        // 重写subTick个数
        int getSubTickCount(double tickStep) override;
    private:
        const FreqPlotter* m_parent;
        double tick_freq_step; // 当前刻度间隔（步长）
    };

    static bool registeredMetaTypes;     // 静态标志位
    static void register_meta_types_once(); // 静态注册函数

    // 公共关键参数
    int m_fft_size;
    PlotMode m_plot_mode;
    FFTDisplayMode m_fft_mode;
    bool fft_size_inited;
    // 公共一般参数
    double m_fsa;
    /* 显示值和绘制值要分开：显示值指窗口显示的横坐标范围，绘制值指是指绘制的点的横坐标值。*/
    int x_min_limit; // 窗口横坐标，即频率，单位Hz，KHz
    int x_max_limit;
    int x_min;
    int x_max;
    int y_min_limit; // 窗口纵坐标，即信号强度，单位dBFs/dBm/dBV
    int y_max_limit;
    int y_min;
    int y_max;
    int bin_min_limit; // 数据点横坐标
    int bin_max_limit;
    int bin_min;
    int bin_max;
    int plot_internal; // ms
    bool plot_paused;
    qint64 last_plot_time;
    qint64 cnt_plot_time;
    // 频谱图参数
    int freq_bin_step; // 频谱图bin间隔
    double line_x;
    double line_y;
    int avg_len;
    int buf_in;
    int buf_out;
    // 瀑布图参数
    bool waterfall_started;
    int map_xsize;
    int map_ysize;
    int time_lower;
    int time_upper;

    // 绘图成员&指针
    QVBoxLayout *layout;
    QCustomPlot *spectrum_plot;
    QCustomPlot *waterfall_plot;
    QCPColorScale *color_scale;
    QCPColorMap *color_map;
    QCPMarginGroup *group;
    QSharedPointer<FreqTicker> m_ticker;
    QElapsedTimer timer_plot;
    QCPColorGradient color_gradient;
    QMarkLine *mark_line_spectrum;
    // 后续可以考虑将sum_vector和avg_buffer进行封装，弄一个循环缓冲区求平均的类，包括avg_len，buf_in，buf_out
    QVector<float> sum_vector;
    QVector<QVector<float>> avg_buffer;

    // TODO：
    //  设置单位？5个单位到底什么意思？只影响计算方式，不影响最终绘图dB，改个label就行
    // freq时间平均，滑动窗，绘图部分不好做，因为fftsize可能会变，和单位一样，需要放到处理模块中，
    // 突然有个思路：自定义内嵌类FreqMap继承QCPColorMap，添加setCellLatestRow，updateMapImageTranslate，shiftRowsBackward，
    // 这样不用破坏qcustomplot的封装完整性，直接将FreqPlotter拷到其他带有qcustomplot文件的工程中就可以使用
    // 改动之后，只需要将原来的成员QCPColorMap *color_map换成FreqMap *color_map，改一下new的地方，其他地方都不用改
    // QCPAxisTicker还有subtick

    void init();
    void init_spectrum();
    void init_waterfall();
    template <typename T>
    void plot_freq_impl(const QVector<T>& freq_data);
    void clamp_xaxis_range(const QCPRange &newRange);
    // 根据FFTDisplayMode和fft更新bin_upper等参数
    void update_bin_ranges();
    int avg_buffer_used();

};


class QMarkLine : public QObject {
Q_OBJECT
public:
    explicit QMarkLine(QCustomPlot *parent = nullptr);

    void set_pos(double mx); // coord position
    void set_text(const QString &text);
    void set_visible(bool vis);
    void set_color(Qt::GlobalColor color);

private:
    QCustomPlot *qcp;
    QCPItemLine *itemLine;
    QCPItemText *textLabel;

    int first_loaded = false;
};

#endif //VLF_FREQPLOTTER_H
