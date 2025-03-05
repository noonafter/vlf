//
// Created by lc on 2025/2/22.
//

// You may need to build the project (run Qt uic code generator) to get "ui_FreqPlotter.h" resolved

#include "freqplotter.h"



FreqPlotter::FreqPlotter(QWidget *parent, bool isComplex) : QWidget(parent) {
    qRegisterMetaType<QVector<float>>("QVector<double>");
    qRegisterMetaType<QVector<float>>("QVector<float>");

    container = parent;
    if(container == nullptr){
        // 将所有属性设置为无效

        return;
    }

    // 默认实数，打开half_range
    if (isComplex) {
        half_range = false;
        shift_range = true;
    } else {
        half_range = true;
        shift_range = false; // 实数时shift_range无效
    }
    freq_bin_step = 1;
    m_fft_size = 512;
    m_bin_state = NeedUpdate;
    bin_lower = 0;
    bin_upper = 0;
    db_lower = -160;
    db_upper = 10;
    map_xsize = 1600;
    map_ysize = 100;
    time_lower = 0;
    time_upper = map_ysize;

    // 获取资源
    layout = new QVBoxLayout(container);
    freq_plot = new QCustomPlot(this);
    time_freq_plot = new QCustomPlot(this);
    color_scale = new QCPColorScale(time_freq_plot);
    color_map = new QCPColorMap(time_freq_plot->xAxis, time_freq_plot->yAxis2);
    group = new QCPMarginGroup(time_freq_plot);

    // 对容器进行布局
    layout->addWidget(freq_plot);
    layout->addWidget(time_freq_plot);
    container->setLayout(layout);

    // 默认显示频谱图
    setPlotMode(FreqMode);

    // 完成画布初始化
    init_freq_plot();
    init_time_freq_plot();
}

FreqPlotter::~FreqPlotter() {

}


void FreqPlotter::init_freq_plot() {
    freq_plot->addGraph();
    freq_plot->graph(0)->setName("rx");
    freq_plot->graph(0)->setPen(QPen(Qt::blue));
    freq_plot->yAxis->setRange(db_lower,db_upper);
    freq_plot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
}

void FreqPlotter::init_time_freq_plot() {
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
    // color scale对齐边界
    time_freq_plot->axisRect()->setMarginGroup(QCP::msTop | QCP::msBottom, group);
    color_scale->setMarginGroup(QCP::msTop | QCP::msBottom, group);

    // 设置color map
    color_map->setColorScale(color_scale);
    color_map->data()->setSize(map_xsize, map_ysize);
    map_data = color_map->data()->get_mdata(); // 必须在setsize之后
    color_map->setInterpolate(true);
    color_map->data()->fill(-160); // moving to QCPColorMapData constructor can speed up launch






    time_freq_plot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
}

void FreqPlotter::setPlotMode(FreqPlotter::PlotMode mode) {
    freq_plot->setVisible(mode == FreqMode);
    time_freq_plot->setVisible(mode == TimeFreqMode);
    m_plot_mode = mode;
}

void FreqPlotter::togglePlotMode() {

    setPlotMode(m_plot_mode == FreqMode ? TimeFreqMode : FreqMode);

}

void FreqPlotter::plot_freq(QVector<double> freq_data) {
    plot_freq_impl(std::move(freq_data));
}

void FreqPlotter::plot_freq(QVector<float> freq_data) {
    plot_freq_impl(std::move(freq_data));
}


template<typename T>
void FreqPlotter::plot_freq_impl(QVector<T> freq_data) {
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

        // 如果在涉及全图重新解算的操作（如color scale重设范围），能够接受原有画面消失，可以注释这一行，加快绘图速度
        // 所有data向后移动一行，空出第一行，
        memmove(map_data + map_xsize, map_data, map_xsize * (map_ysize - 1) * sizeof(map_data[0]));

        // 先确定的map_xsize
        double time_freq_bin_step = double(bin_count)/ map_xsize;
        for (int i = 0; i < map_xsize; i++) {
            idx_vec = lround(bin_lower + i * time_freq_bin_step);
            idx_vec = idx_vec < 0 ? m_fft_size + idx_vec : idx_vec;
            map_data[i] = static_cast<double>(freq_data[idx_vec]);
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

    if (half_range) {
        // 只画前半，索引为0:(N-1)/2;
        bin_lower = 0;
        bin_upper = (m_fft_size - 1) / 2;
    } else if (shift_range) {
        // 画全部，先画后半，索引为(N-1)/2+1-N:-1 和 0:(N-1)/2
        bin_lower = (m_fft_size - 1) / 2 + 1 - m_fft_size;
        bin_upper = (m_fft_size - 1) / 2;
    } else {
        // 画全部，索引为0:(N-1)
        bin_lower = 0;
        bin_upper = m_fft_size - 1;
    }
    // 最终索引只会在(N-1)/2+1-N:-1 和 0:(N-1)
    freq_plot->xAxis->setRange(bin_lower, bin_upper);
    freq_plot->replot();
    // time_freq图跟随设置
    time_freq_plot->xAxis->setRange(bin_lower, bin_upper);
    color_map->data()->setKeyRange(QCPRange(bin_lower, bin_upper));
    time_freq_plot->replot();
    m_bin_state = UptoDate;
}



int FreqPlotter::set_bin_range(int lo, int up) {
    if(lo >= up){
        return 0;
    }

    // 得到当前情况下的bin的端点
    int left,right;
    if(half_range){
        left = 0;
        right = (m_fft_size - 1) / 2;
    } else if(shift_range){
        left = (m_fft_size - 1) / 2 + 1 - m_fft_size;
        right = (m_fft_size - 1) / 2;
    } else{
        left = 0;
        right = m_fft_size - 1;
    }

    // 设置值不能超过端点，否则截断
    if(lo < left){
        lo = left;
    }
    if(up > right){
        up = right;
    }

    // 设置bin range的值，转为Manual模式
    bin_lower = lo;
    bin_upper = up;
    m_bin_state = ManualSet;

    // 检查是否到达两端端点，如果是，转为自动模式
    if(bin_lower == left && bin_upper == right){
        m_bin_state = UptoDate;
    }

    freq_plot->xAxis->setRange(bin_lower, bin_upper);
    freq_plot->replot();

    // time_freq图跟随设置
    time_freq_plot->xAxis->setRange(bin_lower, bin_upper);
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

    if(lo < -160){
        lo = -160;
    }
    if(up > 40){
        up = 40;
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
