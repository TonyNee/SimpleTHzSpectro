#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include "../compat.h"
#include "resample.h"

/** @brief 上采样 (插零) —— 在采样点之间插入零值提高采样率
 *  @param in            输入信号
 *  @param UPSAMPLE_RATE 上采样倍数
 *  @return 上采样后的信号 (长度为 len_in * UPSAMPLE_RATE，需调用者释放)
 *  @note  仅在原始采样点保留原值，其余位置填零
 */
double* upsample(double* in, int UPSAMPLE_RATE) {
    int len_in = static_cast<int>(malloc_usable_size(in) / sizeof(double));
    int len_out = len_in * UPSAMPLE_RATE;
    double* out = (double*)malloc(sizeof(double) * len_out);
    assert(out != NULL);
    // 步骤1: 全部初始化为零
    for (int i = 0; i < len_out; i++) {
        *(out + i) = 0;
    }
    // 步骤2: 隔 UPSAMPLE_RATE 个位置填入原始采样值
    for (int i = 0; i < len_in; i++) {
        *(out + i * UPSAMPLE_RATE) = *(in + i);
    }
    return out;
}

/** @brief 下采样 (抽取) —— 每隔 DOWNSAMPLE_RATE 点取一个值
 *  @param in              输入信号
 *  @param DOWNSAMPLE_RATE 下采样倍数 (需整除len_in)
 *  @return 下采样后的信号 (长度为 len_in / DOWNSAMPLE_RATE，需调用者释放)
 */
double* downsample(double* in, int DOWNSAMPLE_RATE) {
    int len_in = static_cast<int>(malloc_usable_size(in) / sizeof(double));
    assert(!(len_in % DOWNSAMPLE_RATE));
    int len_out = len_in / DOWNSAMPLE_RATE;
    double* out = (double*)malloc(sizeof(double) * len_out);
    assert(out != NULL);
    for (int i = 0; i < len_out; i++) {
        *(out + i) = *(in + i * DOWNSAMPLE_RATE);
    }
    return out;
}
