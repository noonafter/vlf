//
// Created by lc on 2025/2/22.
//

// You may need to build the project (run Qt uic code generator) to get "ui_FreqPlotter.h" resolved

#include "freqplotter.h"


FreqPlotter::FreqPlotter(QWidget *parent) : QWidget(parent) {
    qRegisterMetaType<QVector<double>>("QVector<double>");
    qRegisterMetaType<QVector<float>>("QVector<float>");

    // 默认顺序显示全频谱，即关闭half_range，关闭shift_range
    half_range = false;
    shift_range = false;
    freq_bin_step = 1;
    m_fft_size = 2;
    m_fsa = 2;
    m_bin_state = NeedUpdate;
    bin_lower = 0;
    bin_upper = 0;
    db_minimum = -160;
    db_maximum = 20;
    db_lower = db_minimum;
    db_upper = db_maximum;
    map_xsize = 1600;
    map_ysize = 100;
    time_lower = 0;
    time_upper = map_ysize;
    plot_paused = false;

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

    // 默认显示频谱图
    setPlotMode(FreqMode);

    // 完成画布初始化
    init_freq_plot();
    init_time_freq_plot();

    timer_plot.start();
    last_plot_time = timer_plot.elapsed() - 1000;
    plot_internal = 33;
}

FreqPlotter::~FreqPlotter() {

}


void FreqPlotter::init_freq_plot() {
    freq_plot->xAxis->setTicker(m_ticker);
    freq_plot->xAxis->setRange(0,m_fft_size-1);
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

void FreqPlotter::init_time_freq_plot() {
    time_freq_plot->xAxis->setTicker(m_ticker);
    freq_plot->xAxis->setRange(0,m_fft_size-1);
    time_freq_plot->yAxis->setRange(db_lower,db_upper);
    time_freq_plot->yAxis2->setRange(time_lower,time_upper);
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

void FreqPlotter::setPlotMode(FreqPlotter::PlotMode mode) {
    freq_plot->setVisible(mode == FreqMode);
    time_freq_plot->setVisible(mode == TimeFreqMode);
    m_plot_mode = mode;
}

void FreqPlotter::togglePlotMode() {

    setPlotMode(m_plot_mode == FreqMode ? TimeFreqMode : FreqMode);

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

    int fft_size = freq_data.size();
    // fft_size变化，且为默认情况边界跟随fft_size变化，需要更新bin_lower和bin_upper
    if(fft_size != m_fft_size && m_bin_state != ManualSet){
        m_bin_state = NeedUpdate;
        m_fft_size = fft_size;
    }

    // 得到最新的bin_lower和bin_upper，范围为(N-1)/2+1-N:-1或0:(N-1)/2
    if (m_bin_state == NeedUpdate) {
        update_bin_state();
    }

    // 只画要显示的部分,节省资源
    int bin_count = bin_upper - bin_lower + 1;
    int idx_vec = 0;

    if(m_plot_mode == FreqMode){
        // 先确定freq_bin_step
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
    } else if(m_plot_mode == TimeFreqMode){

        // 所有data向后移动一行，空出第一行
        // 如果在setDataRange或setGradient时，能够接受原有画面消失，可以注释这一行，加快绘图速度
        color_map->data()->shiftRowsBackward(1);

        // 先确定的map_xsize
        double time_freq_bin_step = double(bin_count-1)/ map_xsize;
        for (int i = 0; i < map_xsize; i++) {
            idx_vec = lround(bin_lower + i * time_freq_bin_step);
            idx_vec = idx_vec < 0 ? m_fft_size + idx_vec : idx_vec;
            color_map->data()->setCellLatestRow(i, static_cast<double>(freq_data[idx_vec]));
        }
        color_map->updateMapImageTranslate();
        time_freq_plot->replot();
    }

}

// 在bin跟随fft_size情况下，根据fft_size更新bin
void FreqPlotter::update_bin_state() {
    // 收到空QVector
    if(m_fft_size == 0){
        return;
    }

    get_bin_range_limit(bin_lower, bin_upper);

    // 最终索引只会在(N-1)/2+1-N:(N-1)，但是显示的范围要+1
    freq_plot->xAxis->setRange(bin_lower, bin_upper+1);
    freq_plot->replot();
    // time_freq图跟随设置
    time_freq_plot->xAxis->setRange(bin_lower, bin_upper+1);
    color_map->data()->setKeyRange(QCPRange(bin_lower, bin_upper));
    time_freq_plot->replot();
    m_bin_state = UptoDate;
}


int FreqPlotter::set_bin_range(int lo, int up) {
    if (lo >= up) {
        return 0;
    }

    // 得到当前情况下的bin的端点
    int left, right;
    get_bin_range_limit(left, right);

    // 设置值不能超过端点，否则截断
    if (lo < left) {
        lo = left;
    }
    if (up > right) {
        up = right;
    }

    // 设置bin range的值，转为Manual模式
    bin_lower = lo;
    bin_upper = up;
    m_bin_state = ManualSet;

    // 检查是否到达两端端点，如果是，转为自动模式
    if (bin_lower == left && bin_upper == right) {
        m_bin_state = UptoDate;
    }

    freq_plot->xAxis->setRange(bin_lower, bin_upper+1);
    freq_plot->replot();

    // time_freq图跟随设置
    time_freq_plot->xAxis->setRange(bin_lower, bin_upper+1);
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

bool FreqPlotter::get_half_range() const {
    return half_range;
}

void FreqPlotter::set_half_range(bool half) {
    half_range = half;
}

bool FreqPlotter::get_shift_range() const {
    return shift_range;
}

void FreqPlotter::set_shift_range(bool shift) {
    shift_range = shift;
}

int FreqPlotter::get_fft_size() const {
    return m_fft_size;
}

void FreqPlotter::set_fft_size( int size) {
     m_fft_size = size;
}

double FreqPlotter::get_sample_frequency() const {
    return m_fsa;
}

void FreqPlotter::set_sample_frequency(double fsa) {
    m_fsa = fsa;
}

void FreqPlotter::clamp_xaxis_range(const QCPRange &newRange) {
    QCPRange clampedRange = newRange;

    // 得到当前情况下的bin的端点
    int left, right;
    get_bin_range_limit(left, right);

    // 限制下限不低于 left
    if (clampedRange.lower < left) {
        clampedRange.lower = left;
        clampedRange.upper = clampedRange.size() + left;
    }

    // 限制上限不高于 right
    if (clampedRange.upper > right) {
        clampedRange.upper = right;
        clampedRange.lower = right - clampedRange.size();
    }

    // 如果范围需要修正，则更新
    if (clampedRange != newRange) {
        freq_plot->xAxis->setRange(clampedRange);
        time_freq_plot->xAxis->setRange(clampedRange);
        freq_plot->replot();
        time_freq_plot->replot();
    }
}

void FreqPlotter::get_bin_range_limit(int &left, int &right) const {
    int mid = m_fft_size >> 1;
    if (half_range) {
        left = shift_range ? mid + 1 : 0;
        right = shift_range ? m_fft_size - 1 : mid;
    } else {
        left = shift_range ? mid + 1 - m_fft_size : 0;
        right = shift_range ? mid : m_fft_size - 1;
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
FreqPlotter::FreqTicker::FreqTicker(const FreqPlotter *parent) : m_parent(parent),m_currentStep(0){
    setTickStepStrategy(tssMeetTickCount);
    setTickCount(6);
}

void FreqPlotter::FreqTicker::generate(
        const QCPRange &range,
        const QLocale &locale,
        QChar formatChar,
        int precision,
        QVector<double> &ticks,
        QVector<double> *subTicks,
        QVector<QString> *tickLabels)
{
    // 调用基类生成刻度（索引值）
//    QCPAxisTicker::generate(range, locale, formatChar, precision, ticks, subTicks, tickLabels);

//    // 计算实际频率步长（Hz）
//    if (ticks.size() >= 2) {
//        m_currentStep = (ticks[1] - ticks[0]) * m_parent->m_fsa / m_parent->m_fft_size; // 频率间隔
//    } else {
//        m_currentStep = 0;
//    }

    // Step 1: 计算实际频率范围
    const double lowerFreq = range.lower * m_parent->m_fsa / m_parent->m_fft_size;
    const double upperFreq = range.upper * m_parent->m_fsa / m_parent->m_fft_size;
    const QCPRange freqRange(lowerFreq, upperFreq);

    // Step 2: 计算等分步长（5个间隔生成6个刻度）
    m_currentStep = (freqRange.upper - freqRange.lower) / 5.0;

    // Step 3: 生成主刻度（频率空间）
    ticks.clear();
    for (int i = 0; i < 6; ++i) {
        double freq = freqRange.lower + i * m_currentStep;
        // 转换为索引空间
        double index = freq * m_parent->m_fft_size / m_parent->m_fsa;
        ticks.append(index);
    }

    // Step 4: 强制包含FFT末尾刻度（如果可见）
    const double fsIndex = m_parent->m_fft_size;
    if (range.contains(fsIndex) && !ticks.contains(fsIndex)) {
        ticks.back() = fsIndex; // 替换最后一个刻度为Fs
        m_currentStep = (upperFreq - lowerFreq) / (ticks.size() - 1); // 重新计算步长
    }

    // Step 5: 生成标签
    if (tickLabels) {
        tickLabels->clear();
        for (const auto &tick : ticks) {
            double freq = tick * m_parent->m_fsa / m_parent->m_fft_size;
            int dynamicPrecision = 0;
            if (m_currentStep < 1) dynamicPrecision = 1;
            if (m_currentStep < 0.1) dynamicPrecision = 2;
            if (m_currentStep < 0.01) dynamicPrecision = 3;
            tickLabels->append(QString::number(freq, 'f', dynamicPrecision) + " Hz");
        }
    }

    // Step 6: 生成子刻度（可选）
    if (subTicks) {
        subTicks->clear();
        for (int i = 0; i < ticks.size() - 1; ++i) {
            const double start = ticks[i];
            const double end = ticks[i + 1];
            const double substep = (end - start) / 5.0;
            for (int j = 1; j < 5; ++j) {
                subTicks->append(start + j * substep);
            }
        }
    }
}



QString FreqPlotter::FreqTicker::getTickLabel(double tick, const QLocale &locale, QChar formatChar, int precision) {
    // 根据步长动态计算小数位数
    int dynamicPrecision = 0;
    // 规则：步长<0.1:显示2位，步长<1:显示1位，否则0位
    if(m_currentStep < 0.01){
        dynamicPrecision = 3;
    } else if (m_currentStep < 0.1) {
        dynamicPrecision = 2;
    } else if (m_currentStep < 1) {
        dynamicPrecision = 1;
    } else {
        dynamicPrecision = 0;
    }

    // 计算频率值
    double freq = tick * m_parent->m_fsa / m_parent->m_fft_size;

    // 格式化标签（强制使用 'f' 格式，避免科学计数法）
    return QString::number(freq, 'f', dynamicPrecision) + " Hz";
}




