#pragma once

/**
 * @brief 载波同步：通过累积相关计算LO本振信号的相位
 *
 * 对输入信号与sin/cos本振做CAL_LEN点的累积内积，
 * 通过atan2计算相干相位，用于后续的相干解调。
 *
 * @param in      输入信号
 * @param fc      载波频率(归一化数字频率)
 * @param CAL_LEN 累积相关长度
 * @return        LO的初始相位 phi
 */
double carrier_sync(double* in, double fc, int CAL_LEN);

/**
 * @brief 载波去除：从输入信号中减去重建的载波分量
 *
 * 估计载波幅度和相位后，从原始信号中减去:
 *   out[i] = in[i] - amp * sin(2*pi*fc*i + phi)
 * 用于去除ADC采样数据中的LO泄漏/载波干扰。
 *
 * @param in      输入信号
 * @param fc      载波频率
 * @param CAL_LEN 幅度/相位估计所用长度
 * @return        去除载波后的信号 (需调用者释放内存)
 */
double* carrier_removal(double* in, double fc, int CAL_LEN);
