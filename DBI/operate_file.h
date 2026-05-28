#pragma once

/**
 * @brief 从文本文件加载double数组
 * @param PATH   文件路径
 * @param LENGTH 期望读取的数据点数
 * @return       动态分配的数据数组 (需调用者释放)
 */
double* load_file(const char* PATH, int LENGTH);

/**
 * @brief 从文本文件加载int数组（用于同步头延迟参数）
 * @param PATH   文件路径
 * @param LENGTH 期望读取的数据点数
 * @return       动态分配的整数数组 (需调用者释放)
 */
int* load_file_sync(const char* PATH, int LENGTH);

/**
 * @brief 将double数组写入文本文件
 * @param PATH 输出文件路径
 * @param data 待写入数据
 * @param len  数据长度
 */
void write_file(const char* PATH, double* data, int len);
