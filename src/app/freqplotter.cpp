//
// Created by lc on 2025/2/22.
//

// You may need to build the project (run Qt uic code generator) to get "ui_FreqPlotter.h" resolved

#include "freqplotter.h"


FreqPlotter::FreqPlotter(QWidget *parent)
    : QWidget(parent),
      m_fft_size(2),
      m_plot_mode(Spectrum),
      m_fft_mode(FULL_SEQUENTIAL),
      fft_size_inited(false),
      m_fsa(2),
      bin_lowest_display(0),
      bin_upmost_display(m_fft_size),
      bin_lowest(0),
      bin_upmost(m_fft_size - 1),
      bin_lower(bin_lowest),
      bin_upper(bin_upmost),
      db_minimum(-160),
      db_maximum(20),
      db_lower(db_minimum),
      db_upper(db_maximum),
      plot_internal(33),
      plot_paused(false),
      last_plot_time(0),
      cnt_plot_time(0),
      freq_bin_step(1),
      map_xsize(1600),
      map_ysize(100),
      time_lower(0),
      time_upper(map_ysize),
      layout(nullptr),         // 指针成员显式初始化为nullptr
      freq_plot(nullptr),
      time_freq_plot(nullptr),
      color_scale(nullptr),
      color_map(nullptr),
      group(nullptr)
{
    register_meta_types_once();
    init();
}

FreqPlotter::FreqPlotter(int fft_size, PlotMode plot_mode, FFTDisplayMode fft_mode, QWidget *parent)
    : QWidget(parent),
      m_fft_size(fft_size),
      m_plot_mode(plot_mode),
      m_fft_mode(fft_mode),
      fft_size_inited(true),
      m_fsa(2),
      bin_lowest_display(0),
      bin_upmost_display(m_fft_size),
      bin_lowest(0),
      bin_upmost(m_fft_size - 1),
      bin_lower(bin_lowest),
      bin_upper(bin_upmost),
      db_minimum(-160),
      db_maximum(20),
      db_lower(db_minimum),
      db_upper(db_maximum),
      plot_internal(33),
      plot_paused(false),
      last_plot_time(0),
      cnt_plot_time(0),
      freq_bin_step(1),
      map_xsize(1600),
      map_ysize(100),
      time_lower(0),
      time_upper(map_ysize),
      layout(nullptr),         // 指针成员显式初始化为nullptr
      freq_plot(nullptr),
      time_freq_plot(nullptr),
      color_scale(nullptr),
      color_map(nullptr),
      group(nullptr)
{
    register_meta_types_once();
    init();
}

FreqPlotter::~FreqPlotter() {

}


void FreqPlotter::init() {

    // 获取资源
    layout = new QVBoxLayout(this);
    freq_plot = new QCustomPlot(this);
    time_freq_plot = new QCustomPlot(this);
    color_scale = new QCPColorScale(time_freq_plot);
    color_map = new QCPColorMap(time_freq_plot->xAxis, time_freq_plot->yAxis2);
    group = new QCPMarginGroup(time_freq_plot);
    m_ticker = QSharedPointer<FreqTicker>(new FreqTicker(this));

    // 对容器进行布局
    layout->addWidget(freq_plot);
    layout->addWidget(time_freq_plot);
    this->setLayout(layout);

    // 完成画布初始化
    update_bin_ranges();
    init_spectrum();
    init_waterfall();
    set_plot_mode(m_plot_mode);

    // 启动计时器
    timer_plot.start();
    last_plot_time = timer_plot.elapsed() - 1000;

}

void FreqPlotter::init_spectrum() {
    freq_plot->xAxis->setRange(bin_lowest_display,bin_upmost_display);
    freq_plot->xAxis->setTicker(m_ticker);
    freq_plot->addGraph();
    freq_plot->graph(0)->setName("rx");
    freq_plot->graph(0)->setPen(QPen(Qt::blue));
    freq_plot->yAxis->setRange(db_lower,db_upper);

    // 允许x轴拖拽和缩放
    freq_plot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
    freq_plot->axisRect()->setRangeDrag(Qt::Horizontal);
    freq_plot->axisRect()->setRangeZoom(Qt::Horizontal);
    connect(freq_plot->xAxis,QOverload<const QCPRange &>::of(&QCPAxis::rangeChanged), this, &FreqPlotter::clamp_xaxis_range);
}

void FreqPlotter::init_waterfall() {
    time_freq_plot->xAxis->setRange(bin_lowest_display,bin_upmost_display);
    time_freq_plot->xAxis->setTicker(m_ticker);
    time_freq_plot->yAxis->setRange(db_lower,db_upper);
    time_freq_plot->yAxis2->setRange(time_lower,time_upper);
    color_map->data()->setKeyRange(QCPRange(bin_lower, bin_upper));
    color_map->data()->setValueRange(QCPRange(time_lower, time_upper));

    // 设置color scale
    time_freq_plot->plotLayout()->insertColumn(0);
    time_freq_plot->plotLayout()->addElement(0, 0, color_scale);
    color_scale->setBarWidth(6);
    color_scale->axis()->setTickLabels(false);
    color_scale->setRangeDrag(false);
    color_scale->setRangeZoom(false);
    color_scale->setDataRange(QCPRange(db_lower, db_upper));
    // color scale对齐边界
    time_freq_plot->axisRect()->setMarginGroup(QCP::msTop | QCP::msBottom, group);
    color_scale->setMarginGroup(QCP::msTop | QCP::msBottom, group);

    // 设置color map
    color_map->setColorScale(color_scale);
    color_map->data()->setSize(map_xsize, map_ysize);
    color_map->setInterpolate(true);
    color_map->data()->fill(db_minimum); // moving to QCPColorMapData constructor can speed up launch

    // 允许x轴拖拽和缩放
    time_freq_plot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
    time_freq_plot->axisRect()->setRangeDrag(Qt::Horizontal);
    time_freq_plot->axisRect()->setRangeZoom(Qt::Horizontal);
    connect(time_freq_plot->xAxis,QOverload<const QCPRange &>::of(&QCPAxis::rangeChanged), this, &FreqPlotter::clamp_xaxis_range);
}

int FreqPlotter::get_fft_size() const {
    return m_fft_size;
}

// 是否要加上bin_upmost_display？
void FreqPlotter::set_fft_size(int size) {
    m_fft_size = size;
    fft_size_inited = true;

    update_bin_ranges();

    // 更新横轴并重新绘图
    freq_plot->xAxis->setRange(bin_lowest_display, bin_upmost_display);
    time_freq_plot->xAxis->setRange(bin_lowest_display, bin_upmost_display);
    color_map->data()->setKeyRange(QCPRange(bin_lower, bin_upper));
    freq_plot->replot();
    time_freq_plot->replot();
}

double FreqPlotter::get_sample_rate() const {
    return m_fsa;
}

void FreqPlotter::set_sample_rate(double rate) {
    m_fsa = rate;
}

FreqPlotter::FFTDisplayMode FreqPlotter::get_fft_display_mode() const {
    return m_fft_mode;
}

void FreqPlotter::set_fft_display_mode(FFTDisplayMode mode) {
    m_fft_mode = mode;
    update_bin_ranges();

    // 更新横轴并重新绘图
    freq_plot->xAxis->setRange(bin_lowest_display, bin_upmost_display);
    time_freq_plot->xAxis->setRange(bin_lowest_display, bin_upmost_display);
    color_map->data()->setKeyRange(QCPRange(bin_lower, bin_upper));
    freq_plot->replot();
    time_freq_plot->replot();

}

FreqPlotter::PlotMode FreqPlotter::get_plot_mode() const {
    return m_plot_mode;
}

void FreqPlotter::set_plot_mode(PlotMode mode) {
    freq_plot->setVisible(mode == Spectrum);
    time_freq_plot->setVisible(mode == Waterfall);
    m_plot_mode = mode;
}

void FreqPlotter::toggle_plot_mode() {

    set_plot_mode(m_plot_mode == Spectrum ? Waterfall : Spectrum);

}

void FreqPlotter::plot_freq(const QVector<double>& freq_data) {
    plot_freq_impl(freq_data);
}

void FreqPlotter::plot_freq(const QVector<float>& freq_data) {
    plot_freq_impl(freq_data);
}


template<typename T>
void FreqPlotter::plot_freq_impl(const QVector<T>& freq_data) {

    // 数据推送速度不超过刷新率
    cnt_plot_time = timer_plot.elapsed();
    if(cnt_plot_time - last_plot_time < plot_internal || plot_paused){
        return;
    }
    last_plot_time = cnt_plot_time;

    // 如果用户未手动设置，则自动设置数据长度为fft长度
    int in_size = freq_data.size();
    if(!fft_size_inited){
        set_fft_size(in_size);
    }

    // 根据预设fftsize，对用户输入数据长度进行校验
    if(m_fft_mode < HALF_LOWER){
        if(in_size < m_fft_size){
            qWarning("Input data size is less than fft_size!");
            return;
        }
    }else{
        if(in_size < m_fft_size/2){
            qWarning("Input data size is less than fft_size/2!");
            return;
        }
    }


    // 只画要显示的部分,节省资源
    int bin_count_a = bin_upper - bin_lower + 1;
    int bin_count_b = freq_data.size();
    int bin_count = bin_count_a < bin_count_b ? bin_count_a : bin_count_b;
    int idx_vec = 0;

    if(m_plot_mode == Spectrum){
        // 先确定freq_bin_step，推出plot_size
        int plot_size = bin_count / freq_bin_step;
        QVector<double> x(plot_size), y(plot_size);
        for (int i = 0; i < plot_size; i++) {
            idx_vec = bin_lower + i * freq_bin_step;
            x[i] = idx_vec;
            idx_vec = idx_vec < 0 ? m_fft_size + idx_vec : idx_vec;
            y[i] = static_cast<double>(freq_data[idx_vec]);
        }
        freq_plot->graph(0)->setData(x, y);
        freq_plot->replot();
    } else if(m_plot_mode == Waterfall){

        // 所有data向后移动一行，空出第一行
        // 如果在setDataRange或setGradient时，能够接受原有画面消失，可以注释这一行，加快绘图速度
        color_map->data()->shiftRowsBackward(1);

        // 先确定plot_size，推出time_freq_bin_step
        // 如何确定plot_size：由于map_xsize是确定的（对应bin_count_a），联合bin_count，即可推出plot_size
        int plot_size = map_xsize * bin_count /  bin_count_a;
        double time_freq_bin_step = double(bin_count-1)/ plot_size;
        for (int i = 0; i < plot_size; i++) {
            idx_vec = lround(bin_lower + i * time_freq_bin_step);
            idx_vec = idx_vec < 0 ? m_fft_size + idx_vec : idx_vec;
            color_map->data()->setCellLatestRow(i, static_cast<double>(freq_data[idx_vec]));
        }
        color_map->updateMapImageTranslate();
        time_freq_plot->replot();
    }

}

int FreqPlotter::set_bin_range(int lo, int up) {
    if (lo >= up) {
        return 0;
    }

    // 设置值不能超过端点，否则截断
    if (lo < bin_lowest) {
        lo = bin_lowest;
    }
    if (up > bin_upmost) {
        up = bin_upmost;
    }

    // 设置bin range的值
    bin_lower = lo;
    bin_upper = up;

    freq_plot->xAxis->setRange(bin_lowest_display, bin_upmost_display);
    freq_plot->replot();

    // time_freq图跟随设置
    time_freq_plot->xAxis->setRange(bin_lowest_display, bin_upmost_display);
    color_map->data()->setKeyRange(QCPRange(bin_lower, bin_upper));
    time_freq_plot->replot();
    return 1;
}

int FreqPlotter::set_bin_lower(int lo) {
    return set_bin_range(lo,bin_upper);
}

int FreqPlotter::set_bin_upper(int up) {
    return set_bin_range(bin_lower,up);
}

int FreqPlotter::set_db_range(int lo, int up) {
    if(lo >= up){
        return 0;
    }

    if(lo < db_minimum){
        lo = db_minimum;
    }
    if(up > db_maximum){
        up = db_maximum;
    }

    db_lower = lo;
    db_upper = up;
    freq_plot->yAxis->setRange(db_lower, db_upper);
    freq_plot->replot();

    // time_freq图跟随设置
    time_freq_plot->yAxis->setRange(db_lower, db_upper);
    color_scale->setDataRange(QCPRange(db_lower, db_upper)); // 后续看看这个计算量大不大，可能会导致整个map重新算
    time_freq_plot->replot();
    return 1;
}

int FreqPlotter::set_db_lower(int lo) {
    return set_db_range(lo,db_upper);
}

int FreqPlotter::set_db_upper(int up) {
    return set_db_range(db_lower,up);
}



void FreqPlotter::clamp_xaxis_range(const QCPRange &newRange) {
    QCPRange clampedRange = newRange;

    // 限制下限不低于 bin_lowest
    if (clampedRange.lower < bin_lowest_display) {
        clampedRange.lower = bin_lowest_display;
        clampedRange.upper = clampedRange.size() + bin_lowest_display;
    }

    // 限制上限不高于 bin_upmost
    if (clampedRange.upper > bin_upmost_display) {
        clampedRange.upper = bin_upmost_display;
        clampedRange.lower = bin_upmost_display - clampedRange.size();
    }

    // 如果范围需要修正，则更新
    if (clampedRange != newRange) {
        freq_plot->xAxis->setRange(clampedRange);
        time_freq_plot->xAxis->setRange(clampedRange);
        freq_plot->replot();
        time_freq_plot->replot();
    }
}


void FreqPlotter::set_plot_paused(bool paused) {
    plot_paused = paused;
}

void FreqPlotter::set_frame_per_second(int fps) {
    if (fps <= 0) {
        plot_internal = 1000; // fps最低为1
    } else if(fps > 100){
        plot_internal = 100;  // fps最高为100
    } else {
        plot_internal = 1000 / fps;
    }
}

// FreqTicker实现
FreqPlotter::FreqTicker::FreqTicker(const FreqPlotter *parent) : m_parent(parent), m_freq_step(0){
}


QString FreqPlotter::FreqTicker::getTickLabel(double tick, const QLocale &locale, QChar formatChar, int precision) {
    // 根据步长动态计算小数位数
    int dynamicPrecision = 0;
    // 规则：步长<0.1:显示2位，步长<1:显示1位，否则0位
    if(m_freq_step < 0.01){
        dynamicPrecision = 3;
    } else if (m_freq_step < 0.1) {
        dynamicPrecision = 2;
    } else if (m_freq_step < 1) {
        dynamicPrecision = 1;
    } else {
        dynamicPrecision = 0;
    }

    // 计算频率值
    double freq = tick * m_parent->m_fsa / m_parent->m_fft_size;

    // 格式化标签（强制使用 'f' 格式，避免科学计数法）
    return QString::number(freq, 'f', dynamicPrecision) + " Hz";
}

double FreqPlotter::FreqTicker::getTickStep(const QCPRange &range) {

    // 固定 6 个刻度
    int tickCount = 6;

    // 计算步长
    double tick_step = (range.upper - range.lower) / (tickCount - 1);
    m_freq_step = tick_step * m_parent->m_fsa / m_parent->m_fft_size;
    return tick_step;
}

bool FreqPlotter::registeredMetaTypes = false;

void FreqPlotter::register_meta_types_once() {
    if (!registeredMetaTypes) {
        qRegisterMetaType<QVector<double>>("QVector<double>");
        qRegisterMetaType<QVector<float>>("QVector<float>");
        registeredMetaTypes = true;
    }
}

void FreqPlotter::update_bin_ranges() {
    int mid = m_fft_size >> 1;
    switch (m_fft_mode) {
        case FULL_SEQUENTIAL:
        default:
            bin_lowest = 0;
            bin_upmost = m_fft_size - 1;
            bin_lowest_display = 0;
            bin_upmost_display = m_fft_size;
            break;
        case FULL_SHIFTED:
            bin_lowest = mid + 1 - m_fft_size;
            bin_upmost = mid;
            bin_lowest_display = -mid;
            bin_upmost_display = mid;
            break;
        case HALF_LOWER:
            bin_lowest = bin_lowest_display = 0;;
            bin_upmost = bin_upmost_display = mid;
            break;
        case HALF_UPPER:
            bin_lowest = mid + 1;
            bin_upmost = m_fft_size - 1;
            bin_lowest_display = mid;
            bin_upmost_display = m_fft_size;
            break;
    }

    bin_lower = bin_lowest;
    bin_upper = bin_upmost;
}



