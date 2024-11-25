//
// Created by noisemonitor on 2024/11/21.
//

#include <math.h>
#include "fsk_vlf.h"


bfsk_vlf_s *bfsk_vlf_create(int freq_carrier, int freq_sep, int sps, int sample_rate, int frame_size) {

    bfsk_vlf_s *_q = (bfsk_vlf_s *) malloc(sizeof(bfsk_vlf_s));

    _q->m_fc = freq_carrier;
    _q->m_freq_sep = freq_sep;
    _q->m_sps = sps;
    _q->m_fsa = sample_rate;
    _q->m_nco = nco_crcf_create(LIQUID_VCO);
    // 只开始设置一次
    nco_crcf_set_phase(_q->m_nco, 0.0f);

    _q->m_frame_bit = (int *) malloc(frame_size * sizeof(int));
    _q->m_frame_size = frame_size;
    _q->idx_sample_cnt = 0;

    return _q;
}


int bfsk_vlf_frame_in(bfsk_vlf_s *_q, int *frame, int len){

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


int bfsk_vlf_modulate_block(bfsk_vlf_s *_q, float *buf, int nsamp) {

    float f_1 = (float) (2 * M_PI * _q->m_fc / _q->m_fsa);
    float mf = M_PI * _q->m_freq_sep / _q->m_fsa;
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

        // fsk调制
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

int bfsk_vlf_modulate_block_cplx(bfsk_vlf_s *_q, float complex* buf, int nsamp){

    float f_1 = (float) (2 * M_PI * _q->m_fc / _q->m_fsa);
    float mf = M_PI * _q->m_freq_sep / _q->m_fsa;
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

        // fsk调制
        nco_crcf_cexpf(_q->m_nco, buf+i);
        nco_crcf_set_frequency(_q->m_nco, f_1 + mf * x_cnt);
        nco_crcf_step(_q->m_nco);

        _q->idx_sample_cnt = (_q->idx_sample_cnt + 1) % (_q->m_frame_size * _q->m_sps);

        // 一帧发完了，剩下的sample补为0，并返回1
        if (_q->idx_sample_cnt == 0) {
            for (; i < nsamp; i++)
                buf[i] = 0.0f + I * 0.0f;
            return 1;
        }
    }

    return 0;
}


int bfsk_vlf_destroy(bfsk_vlf_s *_q) {

    if(_q){
        free(_q->m_frame_bit);
        nco_crcf_destroy(_q->m_nco);
        free(_q);
    }

    return 0;
}

