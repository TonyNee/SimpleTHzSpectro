#include <stdlib.h>
#include <assert.h>
#include "../compat.h"
#include "circshift.h"

/** @brief 循环移位 —— 将数组元素循环移动指定偏移量
 *  @param data 输入数据
 *  @param BIAS 偏移量 (正数右移，负数左移，自动取模)
 *  @return 循环移位后的数据 (需调用者释放)
 */
double* circshift(double* data, int BIAS) {
    int len_data = static_cast<int>(malloc_usable_size(data) / sizeof(double));
    // 步骤1: 取模，确保偏移在 [0, len_data) 范围内
    int bias_local = BIAS % len_data;
    double* out = (double*)malloc(sizeof(double) * len_data);
    assert(out != NULL);
    if (bias_local < 0) {
        bias_local = bias_local + len_data;
    }
    // 步骤2: 前半段 —— 取原数组末尾 bias_local 个元素放到开头
    for (int i = 0; i < bias_local; i++) {
        *(out + i) = *(data + len_data - bias_local + i);
    }
    // 步骤3: 后半段 —— 取原数组开头 len_data - bias_local 个元素接在后面
    for (int i = bias_local; i < len_data; i++) {
        *(out + i) = *(data - bias_local + i);
    }
    return out;
}
