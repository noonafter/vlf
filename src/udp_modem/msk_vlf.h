//
// Created by noisemonitor on 2024/11/25.
//

#ifndef VLF_MSK_VLF_H
#define VLF_MSK_VLF_H

#ifdef __cplusplus
extern "C" {
#endif

#include "liquid.h"

typedef struct {
    int m_fc;
    int m_sps;
    float m_init_phase;
    int m_fsa;
    nco_crcf m_nco;

    int *m_frame_bit;
    int m_frame_size;
    int idx_sample_cnt;
}msk_vlf_s;


// 生成msk信号。如果是带通信号，则可以输出real/complex，如果是基带信号（freq_carrier=0），只能输出complex
msk_vlf_s *msk_vlf_create(int freq_carrier, int sps, int ini_phase, int sample_rate, int frame_size);

// 一次输入一帧，后面可以慢慢生成&取sample
//如果frame为空，则随机生成len长的一帧
int msk_vlf_frame_in(msk_vlf_s *_q, int *frame, int len);

// 一次取nsamp个点到buf中，real版本
// 取完一帧后，剩下的sample补为0，并返回1。取完后如果不重置输入，下次再调用默认重新发一遍
int msk_vlf_modulate_block(msk_vlf_s *_q, float* buf, int nsamp);

// 一次取nsamp个点到buf中，complex版本
// 有个问题，为什么头文件中用float complex编译通不过，必须使用liquid_float_complex，但是实现文件中可以使用float complex
// 此函数生成cplx信号的功能，尚未测试
//int bfsk_vlf_modulate_block_cplx(msk_vlf_s *_q, liquid_float_complex* buf, int nsamp);

int msk_vlf_destroy(msk_vlf_s *_q);
#ifdef __cplusplus
}
#endif
#endif //VLF_MSK_VLF_H
