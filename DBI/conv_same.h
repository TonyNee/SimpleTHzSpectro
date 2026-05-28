#pragma once

/**
 * @brief 一维卷积（same模式）：输出与输入等长
 *        自动处理边界填充为0
 * @param in1 输入信号
 * @param in2 卷积核
 * @return    卷积结果 (需调用者释放)
 */
double* conv_same(double* in1, double* in2);
