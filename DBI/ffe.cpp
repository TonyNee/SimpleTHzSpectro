#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include "../compat.h"
#include "conv_same.h"
#include "ffe.h"

/** @brief FFE (Feed-Forward Equalizer) 前馈均衡器，符号级LMS自适应训练
 *  @param in           输入信号 (过采样后)
 *  @param train        训练序列 (符号速率)
 *  @param TAP_LENGTH   均衡器抽头数
 *  @param SYMBOL_WIDTH 每符号采样点数 (过采样倍数)
 *  @param u            LMS步长因子
 *  @return FFT_STR 结构体 (均衡后输出out, 权重w, 误差e, 均衡器输出z)
 *  @note  使用LMS算法在每个符号位置更新抽头权重
 */
FFT_STR FFE(double* in, double* train, int TAP_LENGTH, int SYMBOL_WIDTH, double u) {
    int len_in = static_cast<int>(malloc_usable_size(in) / sizeof(double));
    int len_train = static_cast<int>(malloc_usable_size(train) / sizeof(double));
    assert(len_in == len_train * SYMBOL_WIDTH);

    FFT_STR ffe_out;
    ffe_out.out = (double*)malloc(sizeof(double) * len_train);
    ffe_out.w = (double*)malloc(sizeof(double) * TAP_LENGTH);
    ffe_out.e = (double*)malloc(sizeof(double) * len_in);
    ffe_out.z = (double*)malloc(sizeof(double) * len_in);
    double* d = (double*)malloc(sizeof(double) * TAP_LENGTH);
    assert(ffe_out.out != NULL);
    assert(ffe_out.w != NULL);
    assert(ffe_out.e != NULL);
    assert(ffe_out.z != NULL);
    assert(d != NULL);

    int TAP_BIAS = (TAP_LENGTH - 1) / 2;
    int I_START = floor((float)TAP_BIAS / SYMBOL_WIDTH) + 1;
    int I_END = len_train - floor(TAP_BIAS / SYMBOL_WIDTH) - 2;

    // 步骤1: 初始化权重为零
    for (int t = 0; t < TAP_LENGTH; t++) {
        *(ffe_out.w + t) = 0;
    }

    // 步骤2: LMS自适应迭代 —— 逐个符号更新抽头权重
    for (int i = I_START; i <= I_END; i++) {
        // 步骤2a: 取当前符号对应的输入延迟线
        int D_START = i * SYMBOL_WIDTH - TAP_BIAS - 1;
        for (int t = 0; t < TAP_LENGTH; t++) {
            *(d + t) = *(in + D_START + TAP_LENGTH - t);
        }

        // 步骤2b: 前向计算均衡器输出
        *(ffe_out.z + i) = 0;
        for (int t = 0; t < TAP_LENGTH; t++) {
            *(ffe_out.z + i) = *(ffe_out.z + i) + *(d + t) * *(ffe_out.w + t);
        }

        // 步骤2c: 计算误差 (训练序列 - 均衡输出)
        *(ffe_out.e + i) = *(train + i) - *(ffe_out.z + i);

        // 步骤2d: LMS权重更新
        for (int t = 0; t < TAP_LENGTH; t++) {
            *(ffe_out.w + t) = *(ffe_out.w + t) + u * *(ffe_out.e + i) * *(d + t);
        }
    }

    // 步骤3: 用训练好的权重对全信号做卷积，并在符号点采样
    double* conv_out = conv_same(in, ffe_out.w);

    for (int i = 0; i < len_train; i++) {
        *(ffe_out.out + i) = *(conv_out + i * SYMBOL_WIDTH);
    }

    free(conv_out);
    free(d);

    return(ffe_out);
}

/** @brief MISO-FFE (Multiple-Input Single-Output FFE) 多入单出前馈均衡器
 *  @param in         多通道输入信号 (group_num x len_in)
 *  @param train      训练序列
 *  @param TAP_LENGTH 每通道抽头数
 *  @param u          LMS步长因子
 *  @return MISO_FFT_STR 结构体 (输出out, 权重矩阵w, 误差e, 均衡器输出z)
 *  @note  输入和训练序列长度一致 (已经是符号速率)，多通道同时LMS训练
 */
MISO_FFT_STR MISO_FFE(double** in, double* train, int TAP_LENGTH, double u) {
    int group_num = static_cast<int>(malloc_usable_size(in) / sizeof(*(in)));
    int len_in = static_cast<int>(malloc_usable_size(*(in)) / sizeof(**(in)));
    int len_train = static_cast<int>(malloc_usable_size(train) / sizeof(double));
    assert(len_in == len_train);

    MISO_FFT_STR ffe_out;
    // 步骤1: 分配权重矩阵 (group_num x TAP_LENGTH)
    ffe_out.w = (double**)malloc(sizeof(double*) * group_num);
    assert(ffe_out.w != NULL);
    for (int g = 0; g < group_num; g++) {
        *(ffe_out.w + g) = (double*)malloc(sizeof(double) * TAP_LENGTH);
        assert(*(ffe_out.w + g) != NULL);
    }
    // 分配延迟线矩阵
    double** d = (double**)malloc(sizeof(double*) * group_num);
    assert(d != NULL);
    for (int g = 0; g < group_num; g++) {
        *(d + g) = (double*)malloc(sizeof(double) * TAP_LENGTH);
        assert(*(d + g) != NULL);
    }
    ffe_out.e = (double*)malloc(sizeof(double) * len_in);
    ffe_out.z = (double*)malloc(sizeof(double) * len_in);
    assert(ffe_out.e != NULL);
    assert(ffe_out.z != NULL);

    int TAP_BIAS = floor((float)TAP_LENGTH / 2);
    int I_START = TAP_BIAS + 1;
    int I_END = len_train - TAP_BIAS - 2;

    // 步骤2: 初始化所有权重为零
    for (int g = 0; g < group_num; g++) {
        for (int j = 0; j < TAP_LENGTH; j++) {
            *(*(ffe_out.w + g) + j) = 0;
        }
    }
    for (int g = 0; g < group_num; g++) {
        for (int j = 0; j < len_in; j++) {
            *(ffe_out.e + j) = 0;
        }
    }

    // 步骤3: LMS自适应迭代 —— 所有通道同时更新
    for (int i = I_START; i <= I_END; i++) {
        // 步骤3a: 取当前符号对应的各通道延迟线
        int D_START = i - TAP_BIAS - 1;
        for (int g = 0; g < group_num; g++) {
            for (int t = 0; t < TAP_LENGTH; t++) {
                *(*(d + g) + t) = *(*(in + g) + D_START + TAP_LENGTH - t);
            }
        }

        // 步骤3b: 多通道加权求和得到均衡输出
        *(ffe_out.z + i) = 0;
        for (int g = 0; g < group_num; g++) {
            for (int t = 0; t < TAP_LENGTH; t++) {
                *(ffe_out.z + i) = *(ffe_out.z + i) + *(*(d + g) + t) * *(*(ffe_out.w + g) + t);
            }
        }

        // 步骤3c: 计算误差
        *(ffe_out.e + i) = *(train + i) - *(ffe_out.z + i);

        // 步骤3d: 各通道LMS权重更新
        for (int g = 0; g < group_num; g++) {
            for (int t = 0; t < TAP_LENGTH; t++) {
                *(*(ffe_out.w + g) + t) = *(*(ffe_out.w + g) + t) + u * *(ffe_out.e + i) * *(*(d + g) + t);
            }
        }
    }

    // 步骤4: 用训练好的权重组对各通道做卷积并求和
    ffe_out.out = conv_same(*in, *(ffe_out.w));
    for (int g = 1; g < group_num; g++) {
        double* tmp = conv_same(*(in + g), *(ffe_out.w + g));
        for (int i = 0; i < len_train; i++) {
            *(ffe_out.out + i) = *(ffe_out.out + i) + *(tmp + i);
        }
    }

    // 释放临时内存
    for (int g = 1; g < group_num; g++) {
        free(*(d + g));
    }
    free(d);

    return(ffe_out);
}
