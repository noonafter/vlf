//
// Created by noisemonitor on 2024/11/21.
//

#ifndef VLF_FSK_VLF_H
#define VLF_FSK_VLF_H

#ifdef __cplusplus
extern "C" {
#endif

#include "liquid.h"
    //256+38+35出大问题
#define FRAME_SIZE (256+38+35)

typedef struct {

    int m_fc;
    int m_freq_sep;
    int m_sps;
    int m_fsa;
    nco_crcf m_nco;

    int *m_frame_bit;
    int idx_sample_cnt;

} bfsk_vlf_s;

bfsk_vlf_s *bfsk_vlf_create(int freq_carrier, int freq_sep, int sps, int sample_rate);

// 本来应该由输入的
void bfsk_vlf_modulate_block(bfsk_vlf_s *_q, float* buf, int nsamp);

//void bfsk_vlf_modulate(struct bfsk_vlf_s *_q, float* buf_cnt);

int bfsk_vlf_destroy(bfsk_vlf_s *_q);

#ifdef __cplusplus
}
#endif

#endif //VLF_FSK_VLF_H

