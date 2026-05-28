#pragma once

/**
 * @brief 同步结果结构体
 */
typedef struct {
    double* data;    // 同步对齐后的输出数据
    int syn_head;    // 同步头偏移位置(互相关峰值位置)
}SYNC_STR;

/**
 * @brief 时间同步：通过互相关将输入信号与参考信号对齐
 *
 * 在SYNC_BIAS偏移处截取SYNC_LENGTH长度与ref做互相关，
 * 找到最大相关峰值位置，据此对输入信号做循环移位对齐。
 *
 * @param in            输入信号
 * @param ref           参考信号
 * @param SYNC_BIAS     同步搜索起始偏移
 * @param SYNC_LENGTH   互相关窗口长度
 * @param OUTPUT_LENGTH 输出数据长度
 * @return              同步结果(含对齐数据和同步偏移)
 */
SYNC_STR sync(double* in, double* ref, int SYNC_BIAS, int SYNC_LENGTH, int OUTPUT_LENGTH);
