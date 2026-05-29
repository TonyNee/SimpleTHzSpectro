#include <stdlib.h>
#include <assert.h>
#include "conv_same.h"
#include "../compat.h"

/** @brief 一维卷积 (same模式) —— 输出与输入等长，边界处用部分卷积核计算
 *  @param in1 输入信号
 *  @param in2 卷积核
 *  @return 卷积结果 (与in1等长，需调用者释放)
 *  @note  支持奇数/偶数长度卷积核；边界处使用截断卷积核保证输出长度一致
 */
double* conv_same(double* in1, double* in2) {
    int len_in1 = static_cast<int>(malloc_usable_size(in1) / sizeof(double));
    int len_in2 = static_cast<int>(malloc_usable_size(in2) / sizeof(double));
    double* out = (double*)malloc(sizeof(double) * len_in1);
    assert(out != NULL);

    for (int i = 0; i < len_in1; i++) {
        *(out + i) = 0;
        int len_in2_hf;
        if (len_in2 % 2 == 0) {
            // 卷积核长度为偶数
            len_in2_hf = len_in2 / 2;
            if (i < len_in2_hf - 1) {
                // 左边界: 部分卷积核
                for (int j = 0; j < len_in2_hf + 1 + i; j++) {
                    *(out + i) = *(out + i) + in1[j] * in2[len_in2_hf + i - j];
                }
            }
            else if (i > len_in1 - 1 - len_in2_hf) {
                // 右边界: 部分卷积核
                for (int j = 0; j < len_in2_hf + (len_in1 - 1 - i); j++) {
                    double tmp = in1[len_in1 - 1 - j] * in2[len_in2_hf - (len_in1 - 1 - i) + j];
                    *(out + i) = *(out + i) + in1[len_in1 - 1 - j] * in2[len_in2_hf - (len_in1 - 1 - i) + j];
                }
            }
            else {
                // 中间区域: 完整卷积核
                for (int j = 0; j < len_in2; j++) {
                    *(out + i) = *(out + i) + in1[i - len_in2_hf + 1 + j] * in2[len_in2 - 1 - j];
                }
            }
        }
        else {
            // 卷积核长度为奇数
            len_in2_hf = (len_in2 - 1) / 2;
            if (i < len_in2_hf) {
                // 左边界: 部分卷积核
                for (int j = 0; j < len_in2_hf + 1 + i; j++) {
                    *(out + i) = *(out + i) + in1[j] * in2[len_in2_hf + i - j];
                }
            }
            else if (i > len_in1 - 1 - len_in2_hf) {
                // 右边界: 部分卷积核
                for (int j = 0; j < len_in2_hf + 1 + (len_in1 - 1 - i); j++) {
                    *(out + i) = *(out + i) + in1[len_in1 - 1 - j] * in2[len_in2_hf - (len_in1 - 1 - i) + j];
                }
            }
            else {
                // 中间区域: 完整卷积核
                for (int j = 0; j < len_in2; j++) {
                    *(out + i) = *(out + i) + in1[i - len_in2_hf + j] * in2[len_in2 - 1 - j];
                }
            }
        }
    }
    return out;
}
