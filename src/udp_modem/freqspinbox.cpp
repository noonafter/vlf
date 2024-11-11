//
// Created by noisemonitor on 2024/11/8.
//

// You may need to build the project (run Qt uic code generator) to get "ui_FreqSpinBox.h" resolved

#include "freqspinbox.h"
#include <cmath>

FreqSpinBox::FreqSpinBox(QWidget *parent) :QDoubleSpinBox(parent) {
    this->setKeyboardTracking(false);
    freq_set_lock = true;
    setSingleStep(0.3);
    setRange(9.9,60.0);
    setDecimals(3);
    setValue(9.900);
    setSuffix(" KHz");
    connect(this,QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &FreqSpinBox::onValueChanged);
}

FreqSpinBox::~FreqSpinBox() {


}

void FreqSpinBox::set_freq_set_lock(int lock_state) {

    freq_set_lock = lock_state;
    setSingleStep(freq_set_lock ? 0.3 : 0.1);
    emit valueChanged(value());
}

void FreqSpinBox::onValueChanged(double value) {

    double round_value = freq_set_lock ? 9.9 + std::round((value-9.9)/0.3)*0.3 : value;
    if(round_value > 60.0)
        round_value = 60.0;
    this->setValue(round_value);

}

