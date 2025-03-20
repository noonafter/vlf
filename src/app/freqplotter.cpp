//
// Created by lc on 2025/2/22.
//

// You may need to build the project (run Qt uic code generator) to get "ui_FreqPlotter.h" resolved

#include "freqplotter.h"


FreqPlotter::FreqPlotter(QWidget *parent)
    : QWidget(parent),
      m_fft_size(2),
      m_plot_mode(SPECTRUM),
      m_fft_mode(FULL_SEQUENTIAL),
      fft_size_inited(false),
      m_fsa(2),
      x_min_limit(0),
      x_max_limit(m_fft_size),
      x_min(x_min_limit),
      x_max(x_max_limit),
      y_min_limit(-160),
      y_max_limit(20),
      y_min(y_min_limit),
      y_max(y_max_limit),
      bin_min_limit(0),
      bin_max_limit(m_fft_size - 1),
      bin_min(bin_min_limit),
      bin_max(bin_max_limit),
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
      spectrum_plot(nullptr),
      waterfall_plot(nullptr),
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
      x_min_limit(0),
      x_max_limit(m_fft_size),
      x_min(x_min_limit),
      x_max(x_max_limit),
      y_min_limit(-160),
      y_max_limit(20),
      y_min(y_min_limit),
      y_max(y_max_limit),
      bin_min_limit(0),
      bin_max_limit(m_fft_size - 1),
      bin_min(bin_min_limit),
      bin_max(bin_max_limit),
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
      spectrum_plot(nullptr),
      waterfall_plot(nullptr),
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
    spectrum_plot = new QCustomPlot(this);
    waterfall_plot = new QCustomPlot(this);
    color_scale = new QCPColorScale(waterfall_plot);
    color_map = new QCPColorMap(waterfall_plot->xAxis, waterfall_plot->yAxis2);
    group = new QCPMarginGroup(waterfall_plot);
    m_ticker = QSharedPointer<FreqTicker>(new FreqTicker(this));
    color_gradient = QCPColorGradient::gpSpectrum;
    mark_line_spectrum = new QMarkLine(spectrum_plot);

    // 对容器进行布局
    layout->addWidget(spectrum_plot);
    layout->addWidget(waterfall_plot);
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
    spectrum_plot->xAxis->setRange(x_min, x_max);
    spectrum_plot->xAxis->setTicker(m_ticker);
    spectrum_plot->addGraph();
    spectrum_plot->graph(0)->setName("rx");
    spectrum_plot->graph(0)->setPen(QPen(Qt::blue));
    spectrum_plot->yAxis->setRange(y_min, y_max);

    // 允许x轴拖拽和缩放
    spectrum_plot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
    spectrum_plot->axisRect()->setRangeDrag(Qt::Horizontal);
    spectrum_plot->axisRect()->setRangeZoom(Qt::Horizontal);
    // 仅禁用右边自动边距，保留其他方向自动计算
    spectrum_plot->axisRect()->setAutoMargins(QCP::msLeft | QCP::msTop | QCP::msBottom);
    spectrum_plot->axisRect()->setMargins(QMargins(0, 0, 40, 0));
    connect(spectrum_plot->xAxis, QOverload<const QCPRange &>::of(&QCPAxis::rangeChanged), this, &FreqPlotter::clamp_xaxis_range);
    connect(spectrum_plot, &QCustomPlot::mouseDoubleClick, this, [=](QMouseEvent *event){
        line_x = spectrum_plot->xAxis->pixelToCoord(event->pos().x());
        mark_line_spectrum->set_pos(line_x);
        spectrum_plot->replot();
    });
}

void FreqPlotter::init_waterfall() {
    waterfall_plot->xAxis->setRange(x_min, x_max);
    waterfall_plot->xAxis->setTicker(m_ticker);
    waterfall_plot->yAxis->setRange(y_min, y_max);
    waterfall_plot->yAxis2->setRange(time_lower, time_upper);
    color_map->data()->setKeyRange(QCPRange(bin_min, bin_max));
    color_map->data()->setValueRange(QCPRange(time_lower, time_upper));

    // 设置color scale
    waterfall_plot->plotLayout()->insertColumn(0);
    waterfall_plot->plotLayout()->addElement(0, 0, color_scale);
    color_scale->setBarWidth(6);
    color_scale->axis()->setTickLabels(false);
    color_scale->setRangeDrag(false);
    color_scale->setRangeZoom(false);
    color_scale->setDataRange(QCPRange(y_min, y_max));
    // color scale对齐边界
    waterfall_plot->axisRect()->setMarginGroup(QCP::msTop | QCP::msBottom, group);
    color_scale->setMarginGroup(QCP::msTop | QCP::msBottom, group);

    // 设置color map
    color_map->setColorScale(color_scale);
    color_map->setGradient(color_gradient);
    color_map->data()->setSize(map_xsize, map_ysize);
    color_map->setInterpolate(true);
    color_map->data()->fill(y_min_limit); // moving to QCPColorMapData constructor can speed up launch

    // 允许x轴拖拽和缩放
    waterfall_plot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
    waterfall_plot->axisRect()->setRangeDrag(Qt::Horizontal);
    waterfall_plot->axisRect()->setRangeZoom(Qt::Horizontal);
    // 仅禁用右边自动边距，保留其他方向自动计算
    waterfall_plot->axisRect()->setAutoMargins(QCP::msLeft | QCP::msTop | QCP::msBottom);
    waterfall_plot->axisRect()->setMargins(QMargins(0, 0, 40, 0));
    connect(waterfall_plot->xAxis, QOverload<const QCPRange &>::of(&QCPAxis::rangeChanged), this, &FreqPlotter::clamp_xaxis_range);
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
    spectrum_plot->xAxis->setRange(x_min_limit, x_max_limit);
    waterfall_plot->xAxis->setRange(x_min_limit, x_max_limit);
    color_map->data()->setKeyRange(QCPRange(bin_min, bin_max));
    spectrum_plot->replot();
    waterfall_plot->replot();
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
    spectrum_plot->xAxis->setRange(x_min_limit, x_max_limit);
    waterfall_plot->xAxis->setRange(x_min_limit, x_max_limit);
    color_map->data()->setKeyRange(QCPRange(bin_min, bin_max));
    spectrum_plot->replot();
    waterfall_plot->replot();

}

FreqPlotter::PlotMode FreqPlotter::get_plot_mode() const {
    return m_plot_mode;
}

void FreqPlotter::set_plot_mode(PlotMode mode) {
    spectrum_plot->setVisible(mode == SPECTRUM);
    waterfall_plot->setVisible(mode == WATERFALL);
    m_plot_mode = mode;
}

void FreqPlotter::toggle_plot_mode() {

    set_plot_mode(m_plot_mode == SPECTRUM ? WATERFALL : SPECTRUM);

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
    int bin_count_a = bin_max - bin_min + 1;
    int bin_count_b = freq_data.size();
    int bin_count = bin_count_a < bin_count_b ? bin_count_a : bin_count_b;
    int idx_vec = 0;

    if(m_plot_mode == SPECTRUM && spectrum_plot){
        // 先确定freq_bin_step，推出plot_size
        int plot_size = bin_count / freq_bin_step;
        QVector<double> x(plot_size), y(plot_size);
        for (int i = 0; i < plot_size; i++) {
            idx_vec = bin_min + i * freq_bin_step;
            x[i] = idx_vec;
            idx_vec = idx_vec < 0 ? m_fft_size + idx_vec : idx_vec;
            y[i] = static_cast<double>(freq_data[idx_vec]);
        }
        spectrum_plot->graph(0)->setData(x, y);

        // 更新markline
        if (line_x < bin_min) {
            line_x = bin_min;
        } else if (line_x > bin_max) {
            line_x = bin_max;
        }
        QString txt_label = QString("%1Hz\n%2dBm").arg(m_fsa/m_fft_size*line_x);
        if(spectrum_plot->graph(0)){
            line_y = spectrum_plot->graph(0)->data()->findBegin(line_x)->value;
            txt_label = txt_label.arg((int) line_y);
        }
        mark_line_spectrum->set_text(txt_label);

        // 更新绘图
        spectrum_plot->replot();
    } else if(m_plot_mode == WATERFALL && waterfall_plot && color_map){

        // 所有data向后移动一行，空出第一行
        // 如果在setDataRange或setGradient时，能够接受原有画面消失，可以注释这一行，加快绘图速度
        color_map->data()->shiftRowsBackward(1);

        // 先确定plot_size，推出time_freq_bin_step
        // 如何确定plot_size：由于map_xsize是确定的（对应bin_count_a），联合bin_count，即可推出plot_size
        int plot_size = map_xsize * bin_count /  bin_count_a;
        double time_freq_bin_step = double(bin_count-1)/ plot_size;
        for (int i = 0; i < plot_size; i++) {
            idx_vec = lround(bin_min + i * time_freq_bin_step);
            idx_vec = idx_vec < 0 ? m_fft_size + idx_vec : idx_vec;
            color_map->data()->setCellLatestRow(i, static_cast<double>(freq_data[idx_vec]));
        }
        color_map->updateMapImageTranslate();
        waterfall_plot->replot();
    }

}

int FreqPlotter::set_bin_range(int min, int max) {
    if (min >= max) {
        return 0;
    }

    // 设置值不能超过端点，否则截断
    int lo_diplay, up_display;
    if (min <= bin_min_limit) {
        bin_min = bin_min_limit;
        x_min = x_min_limit;
    }else{
        bin_min = min;
        x_min = min;
    }

    if (max >= bin_max_limit) {
        bin_max = bin_max_limit;
        x_max = x_max_limit;
    } else {
        bin_max = max;
        x_max = max;
    }


    spectrum_plot->xAxis->setRange(x_min, x_max);
    spectrum_plot->replot();

    // time_freq图跟随设置
    waterfall_plot->xAxis->setRange(x_min, x_max);
    color_map->data()->setKeyRange(QCPRange(bin_min, bin_max));
    waterfall_plot->replot();
    return 1;
}

int FreqPlotter::set_bin_min(int min) {
    return set_bin_range(min, bin_max);
}

int FreqPlotter::set_bin_max(int max) {
    return set_bin_range(bin_min, max);
}

int FreqPlotter::set_db_range(int min, int max) {
    if(min >= max){
        return 0;
    }

    if(min < y_min_limit){
        min = y_min_limit;
    }
    if(max > y_max_limit){
        max = y_max_limit;
    }

    y_min = min;
    y_max = max;
    spectrum_plot->yAxis->setRange(y_min, y_max);
    spectrum_plot->replot();

    // time_freq图跟随设置
    waterfall_plot->yAxis->setRange(y_min, y_max);
    color_scale->setDataRange(QCPRange(y_min, y_max)); // 后续看看这个计算量大不大，可能会导致整个map重新算
    waterfall_plot->replot();
    return 1;
}

int FreqPlotter::set_db_min(int min) {
    return set_db_range(min, y_max);
}

int FreqPlotter::set_db_max(int max) {
    return set_db_range(y_min, max);
}

void FreqPlotter::clamp_xaxis_range(const QCPRange &newRange) {
    QCPRange clampedRange = newRange;

    // 限制下限不低于 bin_min_limit
    if (clampedRange.lower < x_min) {
        clampedRange.lower = x_min;
        clampedRange.upper = clampedRange.size() + x_min;
    }

    // 限制上限不高于 bin_max_limit
    if (clampedRange.upper > x_max) {
        clampedRange.upper = x_max;
        clampedRange.lower = x_max - clampedRange.size();
    }

    // 如果范围需要修正，则更新
    if (clampedRange != newRange) {
        spectrum_plot->xAxis->setRange(clampedRange);
        waterfall_plot->xAxis->setRange(clampedRange);
        spectrum_plot->replot();
        waterfall_plot->replot();
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
FreqPlotter::FreqTicker::FreqTicker(const FreqPlotter *parent) : m_parent(parent), tick_freq_step(0){
}


QString FreqPlotter::FreqTicker::getTickLabel(double tick, const QLocale &locale, QChar formatChar, int precision) {
    // 通过 log10 计算步长的量级，动态确定小数位数
    const double step = tick_freq_step;
    int dynamicPrecision = 0;

    if (step > 0) {
        // 核心公式：通过 log10 计算步长的量级指数
        int exp = static_cast<int>(std::floor(std::log10(step)));
        // 将指数转换为小数位数（例如：step=0.01 → exp=-2 → precision=2）
        dynamicPrecision = std::max(0, -exp);
    }

    // 计算频率值
    double freq = tick * m_parent->m_fsa / m_parent->m_fft_size;

    // 格式化标签（强制使用 'f' 格式，避免科学计数法）
    return QString::number(freq, 'f', dynamicPrecision) + " Hz";
}

double FreqPlotter::FreqTicker::getTickStep(const QCPRange &range) {

    // 固定 6 个刻度
    const int tickCount = 6;

    // 计算步长
    double tick_step = (range.upper - range.lower) / (tickCount - 1);
    tick_freq_step = tick_step * m_parent->m_fsa / m_parent->m_fft_size;
    return tick_step;
}

int FreqPlotter::FreqTicker::getSubTickCount(double tickStep) {
    // 固定4个子刻度，将tick段分为5节
    const int subTickCount = 4;
    return subTickCount;
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
            bin_min_limit = 0;
            bin_max_limit = m_fft_size - 1;
            x_min_limit = 0;
            x_max_limit = m_fft_size;
            break;
        case FULL_SHIFTED:
            bin_min_limit = mid + 1 - m_fft_size;
            bin_max_limit = mid;
            x_min_limit = -mid;
            x_max_limit = mid;
            break;
        case HALF_LOWER:
            bin_min_limit = x_min_limit = 0;;
            bin_max_limit = x_max_limit = mid;
            break;
        case HALF_UPPER:
            bin_min_limit = mid + 1;
            bin_max_limit = m_fft_size - 1;
            x_min_limit = mid;
            x_max_limit = m_fft_size;
            break;
    }

    bin_min = bin_min_limit;
    bin_max = bin_max_limit;
    x_min = x_min_limit;
    x_max = x_max_limit;
}

void FreqPlotter::set_palette(int idx){

    /* enum GradientPreset {
         gpGrayscale
        ,gpHot
        ,gpCold
        ,gpNight
        ,gpCandy
        ,gpGeography
        ,gpIon
        ,gpThermal
        ,gpPolar
        ,gpSpectrum
        ,gpJet
        ,gpHues
    }; */

    // 定义枚举项总数
    const int presetCount = 12; // 例如：gpGrayscale(0) ~ gpHues(11)

    // 检查 idx 有效性，超出范围则重置为默认（gpGrayscale）
    if (idx < 0 || idx >= presetCount) {
        idx = 0;
    }

    color_gradient = static_cast<QCPColorGradient::GradientPreset>(idx);
    color_map->setGradient(color_gradient);

}


QMarkLine::QMarkLine(QCustomPlot *parent) : QObject(parent) {
    qcp = parent;
    itemLine = new QCPItemLine(parent);
    textLabel = new QCPItemText(parent);

    itemLine->setPen(QPen(Qt::red));
    itemLine->start->setTypeX(QCPItemPosition::ptPlotCoords);
    itemLine->start->setTypeY(QCPItemPosition::ptAbsolute);
    itemLine->end->setTypeX(QCPItemPosition::ptPlotCoords);
    itemLine->end->setTypeY(QCPItemPosition::ptAbsolute);

    textLabel->setColor(Qt::red);
    textLabel->position->setTypeX(QCPItemPosition::ptPlotCoords);
    textLabel->position->setTypeY(QCPItemPosition::ptAbsolute);

    set_visible(false);

}

void QMarkLine::set_pos(double mx) {

    if(!qcp || !textLabel || !itemLine)
        return;

    double textX = mx;
    int label_height_pixel = textLabel->bottom->pixelPosition().y() - textLabel->top->pixelPosition().y();

//    int label_width_pixel = textLabel->right->pixelPosition().x() - textLabel->left->pixelPosition().x();
//    double mouseX_pixel = qcp->xAxis->coordToPixel(mx);
//    int min_pixel = qcp->axisRect()->left() + 0.5 * label_width_pixel;
//    int max_pixel = qcp->axisRect()->right() - 0.5 * label_width_pixel;
//    if (mouseX_pixel < min_pixel) {
//        textX = qcp->xAxis->pixelToCoord(min_pixel + 3); // plus 3, avoid pixel overlap
//    }
//
//    if (mouseX_pixel > max_pixel) {
//        textX = qcp->xAxis->pixelToCoord(max_pixel - 3);
//    }

    itemLine->start->setCoords(mx, qcp->axisRect()->bottom());
    itemLine->end->setCoords(mx, qcp->axisRect()->top() + label_height_pixel);
    textLabel->position->setCoords(textX, qcp->axisRect()->top() + 0.5 * label_height_pixel);

    if (!first_loaded) {
        set_visible(true);
        first_loaded = true;
    }
}

void QMarkLine::set_text(const QString &text) {
    if(!textLabel)
        return;
    textLabel->setText(text);

}

void QMarkLine::set_visible(bool vis) {
    if(!textLabel || !itemLine)
        return;
    itemLine->setVisible(vis);
    textLabel->setVisible(vis);
}

void QMarkLine::set_color(Qt::GlobalColor color) {
    if(!textLabel || !itemLine)
        return;
    itemLine->setPen(QPen(color));
    textLabel->setColor(color);

}

