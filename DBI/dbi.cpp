#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include "../compat.h"
#include "dbi.h"
#include "conv_same.h"
#include "resample.h"
#include "carrier.h"
#include "sync.h"
#include "ffe.h"
#include "circshift.h"

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

    // 步骤4: 载波去除(下变频) —— 减去重建的LO信号
    printf("carrying out lo_remove ...\n");
    double* dbi_loc[ch_num];
    for (int i = 0; i < ch_num - 1; i++) {
        dbi_loc[i] = carrier_removal(dbi_fil1[i + 1], lo_freq[i] / 2 / fs_dsp, CAL_LEN);
    }
    // 第0通道(低频)基于第1通道的LO相位做载波去除
    for (int i = 0; i < len_dbi; i++) {
        *(dbi_fil1[0] + i) = *(dbi_fil1[0] + i) * cos(lo_phi[0]);
    }
    dbi_loc[ch_num - 1] = dbi_fil1[ch_num - 1];
    printf("finished lo_remove\n\n");

    // 步骤5: 混频后滤波 —— 滤除混频产物
    printf("carrying out aft_filter ...\n");
    double* dbi_fil2[ch_num];
    for (int i = 0; i < ch_num; i++) {
        dbi_fil2[i] = conv_same(dbi_loc[i], ft_after_mixer[0]);
    }
    printf("finished aft_filter\n\n");

    // 步骤6: 上变频 —— 恢复信号到原始频率位置
    printf("carrying out up-conversion ...\n");
    double* dbi_upc[ch_num];
    dbi_upc[0] = dbi_fil2[0];
    for (int i = ch_num - 1; i > 0; i--) {
        dbi_upc[i] = (double*)malloc(sizeof(double) * len_dbi);
        assert(dbi_upc[i] != NULL);
        for (int j = 0; j < len_dbi; j++) {
            *(dbi_upc[i] + j) = *(dbi_fil2[i] + j) * cos(-lo_phi[i - 1] + 2 * M_PI * (lo_freq[i - 1] / fs_dsp) * j);
        }
    }
    printf("finished up-conversion\n\n");

    // 步骤7: MISO-FFE均衡 —— LMS自适应多通道均衡
    printf("carrying out MISO_FFE ...\n");
    MISO_FFT_STR dbi_ffe = MISO_FFE(dbi_upc, dbi_upc[0], len_equalizer, 0.01);
    printf("finished MISO_FFE\n\n");

    // 步骤8: 通道间同步对齐 —— 补偿通道间延迟差
    printf("carrying out sync ...\n");
    SYNC_STR dbi_syn[ch_num];
    for (int i = 0; i < ch_num; i++) {
        int sync_bias = 1000;
        if (i == 0) {
            dbi_syn[i] = sync(dbi_upc[i], dbi_upc[i], sync_bias, 5000, len_dbi - sync_bias - 500);
        } else {
            dbi_syn[i] = sync(dbi_upc[i], dbi_upc[0], sync_bias, 5000, len_dbi - sync_bias - 500);
        }
    }
    printf("finished sync\n\n");

    // 步骤9: 通道求和 —— 合并所有通道
    printf("carrying out channel sum ...\n");
    double* output_data = (double*)malloc(sizeof(double) * len_out);
    assert(output_data != NULL);
    for (int i = 0; i < len_out; i++) {
        *(output_data + i) = 0;
    }
    for (int g = 0; g < ch_num; g++) {
        double* shifted = circshift(dbi_syn[g].data, sync_delay[g]);
        for (int i = 0; i < len_out; i++) {
            *(output_data + i) += *(shifted + i + 2500);
        }
        free(shifted);
    }
    printf("finished channel sum\n\n");

    // 释放临时内存
    for (int i = 0; i < ch_num; i++) {
        free(dbi_usp[i]);
        free(dbi_fil1[i]);
    }
    for (int i = 0; i < ch_num - 1; i++) {
        free(dbi_loc[i]);
        free(dbi_fil2[i]);
    }
    free(dbi_upc[ch_num - 1]);
    for (int i = 0; i < ch_num; i++) {
        free(dbi_syn[i].data);
    }
    free(dbi_ffe.out);
    for (int g = 0; g < ch_num; g++) {
        free(dbi_ffe.w[g]);
    }
    free(dbi_ffe.w);
    free(dbi_ffe.e);
    free(dbi_ffe.z);

    return output_data;
}
