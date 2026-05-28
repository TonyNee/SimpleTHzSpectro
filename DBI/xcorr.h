#pragma once

/**
 * @brief 互相关计算：通过卷积实现两个信号的互相关
 *        result[n] = sum(in1[i] * in2[i+n])
 * @param in1 信号1
 * @param in2 信号2
 * @return    互相关结果序列 (需调用者释放)
 */
double* xcorr(double* in1, double* in2);
