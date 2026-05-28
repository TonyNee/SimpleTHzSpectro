#pragma once

/**
 * @brief 循环移位：将数组元素循环移动BIAS个位置
 *        BIAS>0 向右移(高索引方向), BIAS<0 向左移
 * @param data 输入数组
 * @param BIAS 移位量
 * @return     移位后的数组 (需调用者释放)
 */
double* circshift(double* data, int BIAS);
