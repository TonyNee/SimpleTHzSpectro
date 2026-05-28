#pragma once

/**
 * @brief 单通道FFE（前馈均衡器）输出结构
 */
typedef struct {
    double* out;   // 均衡输出
    double* w;     // 均衡器抽头系数
    double* e;     // 误差信号
    double* z;     // 均衡器中间输出
}FFT_STR;

/**
 * @brief MISO-FFE（多输入单输出前馈均衡器）输出结构
 */
typedef struct {
    double* out;   // 均衡输出
    double** w;    // 多通道均衡器抽头系数 [group_num][TAP_LENGTH]
    double* e;     // 误差信号
    double* z;     // 均衡器中间输出
}MISO_FFT_STR;

/**
 * @brief 单通道前馈均衡器 (FFE)
 *
 * 使用LMS自适应算法训练均衡器系数，通过最小化训练序列误差
 * 来补偿信道失真。每隔SYMBOL_WIDTH点输出一个均衡后的符号。
 *
 * @param in           输入信号
 * @param train        训练序列(期望输出)
 * @param TAP_LENGTH   均衡器抽头数
 * @param SYMBOL_WIDTH 符号宽度(下采样因子)
 * @param u            LMS步长因子
 * @return             均衡结果(含输出/系数/误差)
 */
FFT_STR FFE(double* in, double* train, int TAP_LENGTH, int SYMBOL_WIDTH, double u);

/**
 * @brief 多输入单输出前馈均衡器 (MISO-FFE)
 *
 * 将多路输入信号通过各自的均衡器系数加权求和，
 * 使用LMS算法联合训练所有通道的系数以逼近训练序列。
 *
 * @param in         多路输入信号 [group_num][len_in]
 * @param train      训练序列(期望输出)
 * @param TAP_LENGTH 均衡器抽头数
 * @param u          LMS步长因子
 * @return           均衡结果(含多通道输出/系数/误差)
 */
MISO_FFT_STR MISO_FFE(double** in, double* train, int TAP_LENGTH, double u, int group_num);
