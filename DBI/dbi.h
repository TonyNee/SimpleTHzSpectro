#pragma once

// ======================================================
// DBI (Digital Back-end Integration) 数字后端处理管线参数
// ======================================================

#define ch_num 3           // ADC通道数（3通道：低频/中频/高频）
#define fs_dsp 120         // DSP处理采样率 120GSa/s
#define len_filter 101     // 滤波器长度
#define len_equalizer 1001 // MISO均衡器长度
#define len_cut 500+1000   // 截断长度 = 500(含同步延迟余量) + 1000(均衡器长度)

/**
 * @brief DBI处理主函数：完整的数字后端信号处理管线
 *
 * 处理流程：
 * 1. 上采样（零插入）
 * 2. LO本振相位恢复（carrier_sync）
 * 3. 混频前滤波（conv_same with ft_before_mixer）
 * 4. 下变频 / LO载波去除（carrier_removal）
 * 5. 混频后滤波（conv_same with ft_after_mixer）
 * 6. 上变频
 * 7. MISO-FFE均衡（w_miso多输入单输出前馈均衡）
 * 8. 通道间同步对齐（sync + circshift）
 * 9. 通道求和
 *
 * @param adc_data     3路ADC原始数据 [ch_num][len_in]
 * @param len_out      输出数据长度
 * @param len_in       输入数据长度
 * @param w_miso       MISO均衡器系数 [ch_num][len_equalizer]
 * @param sync_delay   通道间同步延迟 [ch_num]
 * @param ft_before_mixer  混频前滤波器系数
 * @param ft_after_mixer   混频后滤波器系数 [ch_num-1][len_filter]
 * @param lo_freq      本振频率数组 [ch_num-1]
 * @return             处理后的输出数据 (需调用者释放内存)
 */
double* DBI_process(double** adc_data, int len_out, int len_in, double** w_miso,
                    int* sync_delay, double* ft_before_mixer,
                    double** ft_after_mixer, double* lo_freq);
