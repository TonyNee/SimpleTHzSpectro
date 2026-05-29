#include <stdlib.h>
#include <assert.h>
#include "../compat.h"
#include "xcorr.h"

/** @brief 互相关 (cross-correlation) —— 计算两个信号的互相关序列
 *  @param in1 第一个输入信号
 *  @param in2 第二个输入信号
 *  @return 互相关结果 (长度为 len_max*2-1，需调用者释放)
 *  @note  两信号长度不等时自动补零至等长
 */
double* xcorr(double* in1, double* in2) {
    int len_in1 = static_cast<int>(malloc_usable_size(in1) / sizeof(double));
    int len_in2 = static_cast<int>(malloc_usable_size(in2) / sizeof(double));
    double* in1_local;
    double* in2_local;
    int len_max;
    // 步骤1: 长度对齐 —— 较短的信号补零至 max(len_in1, len_in2)
    if (len_in1 > len_in2) {
        len_max = len_in1;
        in1_local = in1;
        in2_local = (double*)malloc(sizeof(double) * len_max);
        assert(in2_local != NULL);
        for (int i = 0; i < len_in2; i++) {
            *(in2_local + i) = *(in2 + i);
        }
        for (int i = len_in2; i < len_in1; i++) {
            *(in2_local + i) = 0;
        }
    }
    else if (len_in1 < len_in2) {
        len_max = len_in2;
        in1_local = (double*)malloc(sizeof(double) * len_max);
        in2_local = in2;
        assert(in1_local != NULL);
        for (int i = 0; i < len_in1; i++) {
            *(in1_local + i) = *(in1 + i);
        }
        for (int i = len_in1; i < len_in2; i++) {
            *(in1_local + i) = 0;
        }
    }
    else {
        len_max = len_in1;
        in1_local = in1;
        in2_local = in2;
    }

    // 步骤2: 计算互相关 (时域滑动点积)
    int len_full = len_max * 2 - 1;
    double* out = (double*)malloc(sizeof(double) * len_full);
    assert(out != NULL);
    for (int i = 0; i < len_full; i++) {
        *(out + i) = 0;
        if (i < len_max) {
            // 前半段: in1的尾部与in2的头部对齐
            for (int j = 0; j <= i; j++) {
                *(out + i) = *(out + i) + *(in1_local + j) * *(in2_local + len_max - 1 - i + j);
            }
        }
        else {
            // 后半段: in1的头部与in2的尾部对齐
            for (int j = 0; j <= len_full - 1 - i; j++) {
                *(out + i) = *(out + i) + *(in2_local + j) * *(in1_local - len_max + 1 + i + j);
            }
        }
    }

    return out;
}
