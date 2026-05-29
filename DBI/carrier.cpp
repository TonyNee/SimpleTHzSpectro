#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include "../compat.h"
#include "carrier.h"

/** @brief 载波同步 —— 通过相干累加估计载波初相
 *  @param in      输入信号
 *  @param fc      归一化载波频率 (fc/fs)
 *  @param CAL_LEN 用于估计的信号长度
 *  @return 载波初始相位 (弧度)
 *  @note  对信号同相/正交分量进行累加，用atan2求相位
 */
double carrier_sync(double* in, double fc, int CAL_LEN) {
    double pi = 3.14159265358979323846264338;
    int len_in = static_cast<int>(malloc_usable_size(in) / sizeof(double));
    assert(len_in >= CAL_LEN);
    double lo_coh_I = 0, lo_coh_Q = 0;
    // 步骤1: 相干累加 —— 分别累加I路(sin)和Q路(cos)分量
    for (int i = 0; i < CAL_LEN; i++) {
        lo_coh_I = lo_coh_I + *(in + i) * sin(2 * pi * fc * i);
        lo_coh_Q = lo_coh_Q + *(in + i) * cos(2 * pi * fc * i);
    }
    // 步骤2: 用atan2计算相位
    double lo_phi = atan2(lo_coh_Q, lo_coh_I);
    return lo_phi;
}

/** @brief 载波去除 —— 估计并减去信号中的单音载波分量
 *  @param in      输入信号
 *  @param fc      归一化载波频率
 *  @param CAL_LEN 用于估计的信号长度
 *  @return 去除载波后的信号 (需调用者释放)
 *  @note  先估计载波幅度和相位，再从原信号中减去重建的载波
 */
double* carrier_removal(double* in, double fc, int CAL_LEN) {
    double pi = 3.14159265358979323846264338;
    int len_in = static_cast<int>(malloc_usable_size(in) / sizeof(double));
    assert(len_in >= CAL_LEN);
    // 步骤1: 相干累加估计幅度和相位
    double lo_coh_I = 0, lo_coh_Q = 0;
    for (int i = 0; i < CAL_LEN; i++) {
        lo_coh_I = lo_coh_I + *(in + i) * sin(2 * pi * fc * i);
        lo_coh_Q = lo_coh_Q + *(in + i) * cos(2 * pi * fc * i);
    }
    double lo_phi = atan2(lo_coh_Q, lo_coh_I);
    double lo_amp = 2 * sqrt(lo_coh_Q * lo_coh_Q + lo_coh_I * lo_coh_I) / CAL_LEN;

    // 步骤2: 从原信号中减去重建的载波分量
    double* out = (double*)malloc(sizeof(double) * len_in);
    assert(out != NULL);
    for (int i = 0; i < len_in; i++) {
        *(out + i) = *(in + i) - lo_amp * sin(2 * pi * fc * i + lo_phi);
    }
    return out;
}
