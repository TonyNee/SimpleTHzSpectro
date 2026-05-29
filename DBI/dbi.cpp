#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include "dbi.h"
#include "conv_same.h"
#include "resample.h"
#include "carrier.h"
#include <QDebug>

/** @brief 调试打印二维数组的前若干元素 */
void debug_print(double** data, int chnum, int printnum) {
    for(int i = 0; i < chnum; ++i) {
        for(int j = 0; j < printnum; ++j) {
            printf("%f, ", *(data[i]+j));
        }
        printf("\n");
    }
}

/** @brief DBI (Digital Bandwidth Interleaving) 主处理流程
 *  @param adc_data   各通道ADC原始数据 (ch_num x len_in)
 *  @param len_out    输出长度
 *  @param len_in     输入长度
 *  @param w_miso     MISO均衡器各通道权重
 *  @param sync_delay 各通道同步延迟
 *  @param ft_before_mixer  混频前滤波器系数
 *  @param ft_after_mixer   混频后滤波器系数 (每通道独立)
 *  @param lo_freq    各LO频率
 *  @return 合并后的DBI输出信号
 */
double* DBI_process(double** adc_data, int len_out, int len_in, double** w_miso, int* sync_delay, double* ft_before_mixer, double** ft_after_mixer, double* lo_freq) {

    int len_dbi = len_in * ch_num;
    int CAL_LEN = 20000;
    if (len_in <= CAL_LEN)
        CAL_LEN = len_in;
    else;

    // 步骤1: 上采样 —— 每通道插零升采样率
    printf("carrying out upsample ...\n");
    double* dbi_usp[ch_num];
    for (int i = 0; i < ch_num; i++) {
        dbi_usp[i] = upsample(adc_data[i], ch_num);
    }
    printf("finished upsample\n\n");

    // 步骤2: LO相位恢复 —— 从相邻通道估计载波相位
    printf("carrying out lo_recover ...\n");
    double lo_phi[ch_num - 1];
    for (int i = 0; i < ch_num - 1; i++) {
        lo_phi[i] = carrier_sync(dbi_usp[i + 1], lo_freq[i] / 2 / fs_dsp, CAL_LEN) * 2;
    }
    printf("finished lo_recover\n\n");

    // 步骤3: 混频前滤波 —— 滤除带外噪声
    printf("carrying out pre_filter ...\n");
    double* dbi_fil1[ch_num];
    for (int i = 0; i < ch_num; i++) {
        dbi_fil1[i] = conv_same(dbi_usp[i], ft_before_mixer);
    }
    printf("finished pre_filter\n\n");

    // 步骤4: 载波去除 —— 消除残余LO泄漏 (通道1和2)
    printf("carrying out lo_removal ...\n");
    double* dbi_rmlo[ch_num];
    for (int i = 0; i < ch_num; i++) {
        dbi_rmlo[i] = dbi_fil1[i];
    }
    dbi_rmlo[1] = carrier_removal(dbi_rmlo[1], lo_freq[1 - 1] / 4 / fs_dsp, CAL_LEN);
    dbi_rmlo[1] = carrier_removal(dbi_rmlo[1], lo_freq[1 - 1] / 2 / fs_dsp, CAL_LEN);
    dbi_rmlo[2] = carrier_removal(dbi_rmlo[2], lo_freq[2 - 1] / 4 / fs_dsp, CAL_LEN);
    dbi_rmlo[2] = carrier_removal(dbi_rmlo[2], lo_freq[2 - 1] / 2 / fs_dsp, CAL_LEN);
    printf("finished lo_removal\n\n");

    // 步骤5: 上变频 —— 将高频通道搬移到对应频段
    printf("carrying up_conversion ...\n");
    double* dbi_mix[ch_num - 1];
    for (int i = 0; i < ch_num - 1; i++) {
        dbi_mix[i] = (double*)malloc(sizeof(double) * len_dbi);
        assert(dbi_mix[i] != NULL);
        for (int t = 0; t < len_dbi; t++) {
            *(dbi_mix[i] + t) = 2 * *(dbi_rmlo[i + 1] + t) * sin(2 * M_PI * lo_freq[i] / fs_dsp * t + lo_phi[i]);
        }
    }
    printf("finished up_conversion\n\n");

    // 步骤6: 混频后滤波 —— 对上变频后的信号进行低通滤波
    printf("carrying out post_filter ...\n");
    double* dbi_fil2[ch_num];
    dbi_fil2[0] = dbi_rmlo[0];
    for (int i = 0; i < ch_num - 1; i++) {
        dbi_fil2[i + 1] = conv_same(dbi_mix[i], ft_after_mixer[i]);
    }
    printf("finished post_filter\n\n");

    // 步骤7: MISO均衡 —— 补偿各通道的频率响应不一致
    printf("carrying equalize ...\n");
    double* dbi_eq[ch_num];
    for (int i = 0; i < ch_num; i++) {
        dbi_eq[i] = (double*)malloc(sizeof(double) * len_dbi);
        assert(dbi_eq[i] != NULL);
        dbi_eq[i] = conv_same(dbi_fil2[i], w_miso[i]);
    }
    printf("finished equalize\n\n");

    // 步骤8: 时域对齐 —— 按同步延迟偏移各通道数据
    printf("carrying synchronize ...\n");
    double* dbi_sync[ch_num];
    for (int i = 0; i < ch_num; i++) {
        dbi_sync[i] = (double*)malloc(sizeof(double) * len_out);
        assert(dbi_sync[i] != NULL);
        for (int t = 0; t < len_out; t++) {
            *(dbi_sync[i] + t) = *(dbi_eq[i] + t + sync_delay[i] + 2000);
        }
    }
    printf("finished synchronize\n\n");

    // 步骤9: 通道叠加 —— 所有通道信号相加得到最终宽带信号
    printf("carrying add ...\n");
    double* output = (double*)malloc(sizeof(double) * len_out);
    assert(output != NULL);
    for (int t = 0; t < len_out; t++) {
        *(output + t) = 0;
        for (int i = 0; i < ch_num; i++) {
            *(output + t) = *(output + t) + *(dbi_sync[i] + t);
        }
    }
    printf("finished add\n\n");

    // 释放临时内存
    for (int i = 0; i < ch_num; i++) {
        free(dbi_usp[i]);
        free(dbi_fil1[i]);
        free(dbi_eq[i]);
        free(dbi_sync[i]);
    }
    for (int i = 1; i < ch_num; i++) {
        free(dbi_rmlo[i]);
        free(dbi_fil2[i]);
    }
    for (int i = 0; i < ch_num - 1; i++) {
        free(dbi_mix[i]);
    }

    return output;
}
