//
// Created by noisemonitor on 2024/11/8.
//

#ifndef VLF_FREQSPINBOX_H
#define VLF_FREQSPINBOX_H

#include <QDoubleSpinBox>


class FreqSpinBox : public QDoubleSpinBox {
Q_OBJECT

public:
    explicit FreqSpinBox(QWidget *parent = nullptr);

    ~FreqSpinBox() override;
public slots:
    void set_freq_set_lock(int lock_state);

private slots:
    void onValueChanged(double value);
private:
    int freq_set_lock;
};


#endif //VLF_FREQSPINBOX_H
