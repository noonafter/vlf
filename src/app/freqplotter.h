//
// Created by lc on 2025/2/22.
//

#ifndef VLF_FREQPLOTTER_H
#define VLF_FREQPLOTTER_H

#include <QWidget>
#include "qcustomplot.h"

// 支持两种绘图模式：功率谱图；瀑布图，见PlotMode
// 支持模式动态切换，两种模式均通过槽函数plot_freq进行绘制
// 支持4种FFT显示模式：见FFTDisplayMode
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
// 需要对四种模式情况进行测试，查看xAxis的坐标值是否正确
// 后续对set_fsa函数进行更改，加上默认单位

class FreqPlotter : public QWidget {
Q_OBJECT

public:
    enum PlotMode {
        Spectrum = 0, // 绘制功率谱图，默认情况
        Waterfall     // 绘制瀑布图
    };

    enum FFTDisplayMode {
        FULL_SEQUENTIAL = 0, // 展示整个FFT点数，顺序展示，默认情况
        FULL_SHIFTED,        // 展示整个FFT点数，上下频谱移位，常用于复数信号。
        HALF_LOWER,          // 只展示下半部分FFT点数，常用于实数信号
        HALF_UPPER           // 只展示上半部分FFT点数，较少使用
    };

// 提升为FreqPlotter即可使用
    explicit FreqPlotter(QWidget *parent = nullptr);

    explicit FreqPlotter(int fft_size, PlotMode plot_mode = Spectrum, FFTDisplayMode fft_mode = FULL_SEQUENTIAL,
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

    int set_bin_range(int lo, int up);
    int set_bin_lower(int lo);
    int set_bin_upper(int up);
    int set_db_range(int lo, int up);
    int set_db_lower(int lo);
    int set_db_upper(int up);
    void set_plot_paused(bool paused);
    void set_frame_per_second(int fps);

    void toggle_plot_mode();
    void plot_freq(const QVector<double>& freq_data);
    void plot_freq(const QVector<float>& freq_data);
    // 后续考虑加上直接float *和size的plot版本，跨线程会有风险



private:

    class FreqTicker : public QCPAxisTicker {
    public:
        explicit FreqTicker(const FreqPlotter* parent);
    protected:
        // 重写 generate 以捕获当前刻度步长
        double getTickStep(const QCPRange &range) override;
        // 动态调整小数位数
        QString getTickLabel(double tick, const QLocale &locale, QChar formatChar, int precision) override;
    private:
        const FreqPlotter* m_parent;
        double m_freq_step; // 当前刻度间隔（步长）
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
    int bin_lowest_display; // 需要显示的bin上界
    int bin_upmost_display; // 需要显示的bin上界
    int bin_lowest; // 能够绘制的bin下界
    int bin_upmost; // 能够绘制的bin上界
    int bin_lower;  // 需要绘制的bin下界
    int bin_upper;  // 需要绘制的bin上界
    int db_minimum;
    int db_maximum;
    int db_lower;
    int db_upper;
    int plot_internal; // ms
    bool plot_paused;
    qint64 last_plot_time;
    qint64 cnt_plot_time;
    // 频谱图参数
    int freq_bin_step; // 频谱图bin间隔
    // 瀑布图参数
    int map_xsize;
    int map_ysize;
    int time_lower;
    int time_upper;

    // 绘图成员&指针
    QVBoxLayout *layout;
    QCustomPlot *freq_plot;
    QCustomPlot *time_freq_plot;
    QCPColorScale *color_scale;
    QCPColorMap *color_map;
    QCPMarginGroup *group;
    QSharedPointer<FreqTicker> m_ticker;
    QElapsedTimer timer_plot;


    // TODO：加上waterfall一起调
    //  设置单位？5个单位到底什么意思？只影响计算方式，不影响最终绘图dB，改个label就行
    // freq时间平均，滑动窗
    // waterfall加色卡
    // bin_upmost是指的bin_upper能取的上界，但是显示的时候，还需要一个bin_upmost_display，需要测试4中模式上下边境是否正确
    // 内嵌类继承QCPAxis，重写draw
    // 突然有个思路：自定义内嵌类FreqMap继承QCPColorMap，添加setCellLatestRow，updateMapImageTranslate，shiftRowsBackward，
    // 这样不用破坏qcustomplot的封装完整性，直接将FreqPlotter拷到其他带有qcustomplot文件的工程中就可以使用
    // 改动之后，只需要将原来的成员QCPColorMap *color_map换成FreqMap *color_map，改一下new的地方，其他地方都不用改


    void init();
    void init_spectrum();
    void init_waterfall();
    template <typename T>
    void plot_freq_impl(const QVector<T>& freq_data);
    void clamp_xaxis_range(const QCPRange &newRange);
    // 根据FFTDisplayMode和fft更新bin_upper等参数
    void update_bin_ranges();

};


#endif //VLF_FREQPLOTTER_H
