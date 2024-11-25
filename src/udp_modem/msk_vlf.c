//
// Created by noisemonitor on 2024/11/25.
//

#include "msk_vlf.h"
#include <math.h>

msk_vlf_s *msk_vlf_create(int freq_carrier, int sps, int ini_phase, int sample_rate, int frame_size) {

    msk_vlf_s *_q = ( msk_vlf_s *)malloc(sizeof(msk_vlf_s));
    _q->m_fc = freq_carrier;
    _q->m_sps = sps;
    _q->m_init_phase = (ini_phase%4) * M_PI_2;
    _q->m_fsa = sample_rate;
    _q->m_nco = nco_crcf_create(LIQUID_VCO);
    nco_crcf_set_phase(_q->m_nco, _q->m_init_phase);

    _q->m_frame_bit = (int *) malloc(frame_size * sizeof(int));
    _q->m_frame_size = frame_size;
    _q->idx_sample_cnt = 0;

    return _q;
}

int msk_vlf_frame_in(msk_vlf_s *_q, int *frame, int len) {

    if(len > _q->m_frame_size)
        return -1;

    if(frame == NULL){
        // NULL表示随机生成
        for (int i = 0; i < len; i++) {
            _q->m_frame_bit[i] = rand() % 2;
        }
    } else {
        for (int i = 0; i < len; i++) {
            _q->m_frame_bit[i] = (int)(frame[i] > 0);
        }
    }

    _q->idx_sample_cnt = 0;

    return len;
}

int msk_vlf_modulate_block(msk_vlf_s *_q, float *buf, int nsamp) {

    float f_1 = (float) (2 * M_PI * _q->m_fc / _q->m_fsa);
    float mf = M_PI_2  / _q->m_sps;
    int i;
    int idx_bit_cnt;
    int bit_cnt;
    float x_cnt;

    for (i = 0; i < nsamp; i++) {
        // 得到当前bit索引
        idx_bit_cnt = _q->idx_sample_cnt / _q->m_sps;
        // 得到当前bit
        bit_cnt = _q->m_frame_bit[idx_bit_cnt];
        // 映射
        x_cnt = bit_cnt > 0 ? 1.0f : -1.0f;

        // msk调制
        buf[i] = nco_crcf_cos(_q->m_nco);
        nco_crcf_set_frequency(_q->m_nco, f_1 + mf * x_cnt);
        nco_crcf_step(_q->m_nco);

        _q->idx_sample_cnt = (_q->idx_sample_cnt + 1) % (_q->m_frame_size * _q->m_sps);

        // 一帧发完了，剩下的sample补为0，并返回1
        if (_q->idx_sample_cnt == 0) {
            for (; i < nsamp; i++)
                buf[i] = 0.0f;
            return 1;
        }
    }

    return 0;
}

int msk_vlf_destroy(msk_vlf_s *_q) {

    if(_q){
        free(_q->m_frame_bit);
        nco_crcf_destroy(_q->m_nco);
        free(_q);
    }

    return 1;
}

