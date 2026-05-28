#pragma once

/**
 * @brief 上采样：在每两个原始采样点之间插入UPSAMPLE_RATE-1个零
 * @param in            输入信号
 * @param UPSAMPLE_RATE 上采样倍数
 * @return              上采样后的信号 (需调用者释放内存)
 */
double* upsample(double* in, int UPSAMPLE_RATE);

/**
 * @brief 下采样：每隔DOWNSAMPLE_RATE个点取一个值
 * @param in              输入信号
 * @param DOWNSAMPLE_RATE 下采样倍数
 * @return                下采样后的信号 (需调用者释放内存)
 */
double* downsample(double* in, int DOWNSAMPLE_RATE);
