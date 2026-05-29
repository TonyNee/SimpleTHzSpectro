#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "../compat.h"
#include "xcorr.h"
#include "circshift.h"
#include "sync.h"

/** @brief 时域同步 —— 通过互相关将输入信号与参考信号对齐
 *  @param in            输入信号
 *  @param ref           参考信号 (用于互相关)
 *  @param SYNC_BIAS     同步搜索起始偏移
 *  @param SYNC_LENGTH   用于互相关的信号长度
 *  @param OUTPUT_LENGTH 输出信号长度
 *  @return 同步结果结构体 (包含对齐后数据和同步头位置)
 */
SYNC_STR sync(double* in, double* ref, int SYNC_BIAS, int SYNC_LENGTH, int OUTPUT_LENGTH) {
    assert(malloc_usable_size(in) / sizeof(double) >= SYNC_BIAS + OUTPUT_LENGTH);
    // 步骤1: 截取待同步段
    double* sync_in = (double*)malloc(sizeof(double) * SYNC_LENGTH);
    assert(sync_in != NULL);
    for (int i = 0; i < SYNC_LENGTH; i++) {
        *(sync_in + i) = *(in + SYNC_BIAS + i);
    }
    // 步骤2: 计算互相关
    double* syn = xcorr(sync_in, ref);
    int syn_len = static_cast<int>(malloc_usable_size(syn) / sizeof(double));
    // 步骤3: 取绝对值，用于峰值检测
    for (int i = 0; i < syn_len; i++) {
        if (*(syn + i) < 0) {
            *(syn + i) = -*(syn + i);
        }
    }
    // 步骤4: 搜索互相关峰值位置，确定时延
    int max_location = 0;
    double max_value = 0;
    for (int i = 0; i < syn_len; i++) {
        if (*(syn + i) > max_value) {
            max_value = *(syn + i);
            max_location = i;
        }
    }
    // 步骤5: 计算同步头偏移并进行循环移位对齐
    SYNC_STR sync_out;
    sync_out.syn_head = max_location - (syn_len - 1) / 2;
    double* in_shift = circshift(in, -sync_out.syn_head);
    // 步骤6: 截取对齐后的输出段
    sync_out.data = (double*)malloc(sizeof(double) * OUTPUT_LENGTH);
    assert(sync_out.data != NULL);
    for (int i = 0; i < OUTPUT_LENGTH; i++) {
        *(sync_out.data + i) = *(in_shift + SYNC_BIAS + i);
    }
    // 释放临时内存
    free(sync_in);
    free(syn);
    free(in_shift);
    return sync_out;
}
