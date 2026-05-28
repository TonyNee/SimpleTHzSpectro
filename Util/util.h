#ifndef UTIL_H
#define UTIL_H

#include <QVector>

// 全局信号处理参数
#define t_sample 8.333333e-12   // 采样周期: 8.333ps (对应120GSa/s采样率)
#define Tpmax 30e-12            // 最大脉冲宽度: 30ps

/**
 * @brief 重采样函数：有理因子p/q重采样
 * @param input 输入数据
 * @param p     上采样因子(分子)
 * @param q     下采样因子(分母), 实际倍率 = p/q
 * @return      重采样后的数据 (长度 ≈ input.size() * p / q)
 */
QVector<float> resample(const QVector<float>& input, int p, int q);

/**
 * @brief 插值函数：对输入数据进行times倍插值
 * @param before 原始数据
 * @param after  插值结果(输出)
 * @param times  插值倍数
 */
void performInterpolation(const QVector<float>& before, QVector<float>& after, int times);

/**
 * @brief 一维三次样条插值
 * @param x  已知点x坐标
 * @param y  已知点y坐标
 * @param xi 待插值点x坐标
 * @return   插值结果y坐标
 */
QVector<float> interp1(const QVector<float>& x, const QVector<float>& y, const QVector<float>& xi);

/**
 * @brief 寻找峰值：检测信号中所有局部极大值点
 *        判断条件: data[i] > data[i-1] && data[i] > data[i+1] && data[i] > minHeight
 * @param data      输入信号数据
 * @param minHeight 最小峰值高度阈值
 * @return          所有满足条件的峰值索引列表
 */
QVector<int> findPeaks(const QVector<float>& data, float minHeight);

#endif // UTIL_H
