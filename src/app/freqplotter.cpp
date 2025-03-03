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
    x_step = 1;
    m_fft_size = 512;
    m_bin_state = NeedUpdate;
    bin_lower = 0;
    bin_upper = 0;

    // 获取资源
    layout = new QVBoxLayout(container);
    freq_plot = new QCustomPlot(this);
    time_freq_plot = new QCustomPlot(this);

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
    freq_plot->xAxis->setLabel("Frequency (Hz)");
    freq_plot->yAxis->setLabel("Magnitude");
    freq_plot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
}

void FreqPlotter::init_time_freq_plot() {
    time_freq_plot->addGraph();
    time_freq_plot->xAxis->setLabel("Time (s)");
    time_freq_plot->yAxis->setLabel("Frequency (Hz)");
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

    // 只画要显示的部分
    int bin_count = bin_upper - bin_lower + 1;
    int plot_size = bin_count / x_step;
    int idx_vec = 0;
    QVector<double> x(plot_size), y(plot_size);
    for (int i = 0; i < plot_size; i++) {
        idx_vec = bin_lower + i * x_step;
        x[i] = idx_vec;
        idx_vec = idx_vec < 0 ? m_fft_size + idx_vec : idx_vec;
        y[i] = static_cast<double>(freq_data[idx_vec]);
    }
    freq_plot->graph(0)->setData(x, y);
    freq_plot->replot();
}


void FreqPlotter::update_bin_state() {
    // 收到空QVector
    if(m_fft_size == 0){
        return;
    }

    if (half_range) {
        // 只画前半
        bin_lower = 0;
        bin_upper = (m_fft_size - 1) / 2;
    } else if (shift_range) {
        // 画全部，先画后半
        bin_lower = (m_fft_size - 1) / 2 + 1 - m_fft_size;
        bin_upper = (m_fft_size - 1) / 2;
    } else {
        // 画全部
        bin_lower = 0;
        bin_upper = m_fft_size - 1;
    }
    freq_plot->xAxis->setRange(bin_lower, bin_upper);
    freq_plot->yAxis->setRange(0, 100);
    freq_plot->replot();
    m_bin_state = UptoDate;
}

