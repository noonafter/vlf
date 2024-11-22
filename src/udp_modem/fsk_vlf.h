//
// Created by noisemonitor on 2024/11/21.
//

#ifndef VLF_FSK_VLF_H
#define VLF_FSK_VLF_H

#ifdef __cplusplus
extern "C" {
#endif

#include "liquid.h"

// 生成基带/带通BFSK信号，以real/complex形式输出。一次输入一帧，后续可分多次生成&取采样点
// example:
// bfskmod = bfsk_vlf_create(f_c, f_sep, sps, fsa, FRAME_SIZE);
// bfsk_vlf_frame_in(bfskmod, NULL, FRAME_SIZE);

typedef struct {
    int m_fc;
    int m_freq_sep;
    int m_sps;
    int m_fsa;
    nco_crcf m_nco;

    int *m_frame_bit;
    int m_frame_size;
    int idx_sample_cnt;
} bfsk_vlf_s;

// 生成bfsk信号。如果是带通信号，则可以输出real/complex，如果是基带信号（freq_carrier=0），只能输出complex
bfsk_vlf_s *bfsk_vlf_create(int freq_carrier, int freq_sep, int sps, int sample_rate, int frame_size);

// 一次输入一帧，后面可以慢慢生成&取sample
//如果frame为空，则随机生成len长的一帧
int bfsk_vlf_frame_in(bfsk_vlf_s *_q, int *frame, int len);

// 一次取nsamp个点到buf中，real版本
// 取完一帧后，剩下的sample补为0，并返回1。取完后如果不重置输入，下次再调用默认重新发一遍
int bfsk_vlf_modulate_block(bfsk_vlf_s *_q, float* buf, int nsamp);

// 一次取nsamp个点到buf中，complex版本
// 有个问题，为什么头文件中用float complex编译通不过，必须使用liquid_float_complex，但是实现文件中可以使用float complex
// 此函数生成cplx信号的功能，尚未测试
int bfsk_vlf_modulate_block_cplx(bfsk_vlf_s *_q, liquid_float_complex* buf, int nsamp);

int bfsk_vlf_destroy(bfsk_vlf_s *_q);

#ifdef __cplusplus
}
#endif

#endif //VLF_FSK_VLF_H

