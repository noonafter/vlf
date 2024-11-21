//
// Created by noisemonitor on 2024/11/21.
//

#include <math.h>
#include "fsk_vlf.h"


struct bfsk_vlf_s *bfsk_vlf_create(int freq_carrier, int freq_sep, int sps, int sample_rate) {

    struct bfsk_vlf_s *_q = (struct bfsk_vlf_s *) malloc(sizeof (struct bfsk_vlf_s));
    _q->m_fc = freq_carrier;
    _q->m_freq_sep = freq_sep;
    _q->m_sps = sps;
    _q->m_fsa = sample_rate;
    _q->m_nco = nco_crcf_create(LIQUID_VCO);

    _q->m_frame_bit = (int*)malloc(FRAME_SIZE*sizeof(int));
    _q->idx_sample_cnt = 0;

    // 本来应该由输入的，暂时由内部生成
    for(int i = 0; i < FRAME_SIZE; i++){
        _q->m_frame_bit[i] = rand() % 2;
    }

    return _q;
}

void bfsk_vlf_modulate_block(struct bfsk_vlf_s *_q, float *buf, int nsamp) {

    float f_1 = (float) (2 * M_PI * _q->m_fc / _q->m_fsa);
    float mf = M_PI * _q->m_freq_sep / _q->m_fsa;
    int i;

    nco_crcf_set_phase(_q->m_nco, 0.0f);
    for (i = 0; i < nsamp; i++) {
        // 得到当前bit索引
        int idx_bit_cnt = _q->idx_sample_cnt / _q->m_sps;
        // 得到当前bit
        int bit_cnt = _q->m_frame_bit[idx_bit_cnt];
        // 映射
        float x_cnt = bit_cnt > 0 ? 1.0f : -1.0f;

        // fsk调制
        buf[i] = nco_crcf_cos(_q->m_nco);
        nco_crcf_set_frequency(_q->m_nco, f_1 + mf * x_cnt);
        nco_crcf_step(_q->m_nco);

        _q->idx_sample_cnt = (_q->idx_sample_cnt + 1) % (FRAME_SIZE * _q->m_sps);
        // 如果一帧发完了，重新rand一帧
        if(!_q->idx_sample_cnt){
            for(i = 0; i < FRAME_SIZE; i++){
                _q->m_frame_bit[i] = rand() % 2;
            }
        }
    }

}

//void bfsk_vlf_modulate(struct bfsk_vlf_s *_q, float* buf_cnt){
//
//}

int bfsk_vlf_destroy(struct bfsk_vlf_s *_q) {

    free(_q->m_frame_bit);
    nco_crcf_destroy(_q->m_nco);
    free(_q);

    return 1;
}

