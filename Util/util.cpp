#include "util.h"
#include "spline.h"
#include <algorithm>
#include <cmath>
#include <QDebug>

QVector<float> resample(const QVector<float>& input, int p, int q) {
    int N = input.size();
    if (N < 3) return input;

    // 生成已知点x坐标 (1..N)
    QVector<float> x;
    for (int i = 1; i <= N; ++i) {
        x.push_back(static_cast<float>(i));
    }

    // 生成新插值点 xi，从 1 到 N，每隔 q/p 取一个点
    QVector<float> xi;
    for (int i = 0; i <= (N - 1) * p / q; ++i) {
        float xi_val = 1 + static_cast<float>(i) * q / p;
        xi.push_back(xi_val);
    }

    // 使用三次样条插值（替换原Boost b-spline）
    QVector<float> interpolated;
    SplineSpace::Spline spline(x.data(), input.data(), N, SplineSpace::GivenSecondOrder);
    for (float xi_val : xi) {
        float y_val;
        if (spline.SinglePointInterp(xi_val, y_val)) {
            interpolated.push_back(y_val);
        } else {
            interpolated.push_back(0.0f);
        }
    }
    return interpolated;
}

void performInterpolation(const QVector<float>& before, QVector<float>& after, int times) {
    int N = before.size();
    if (N < 3) {
        after = before;
        return;
    }

    // 生成已知点x坐标
    QVector<float> x;
    for (int i = 1; i <= N; ++i) {
        x.push_back(static_cast<float>(i));
    }

    // 生成插值点 xi，步长为 1/times
    QVector<float> xi;
    for (int i = 0; i <= N * times - times; ++i) {
        float x_val = 1 + static_cast<float>(i) / times;
        xi.push_back(x_val);
    }

    // 使用三次样条插值（替换原Boost b-spline）
    after.clear();
    SplineSpace::Spline spline(x.data(), before.data(), N, SplineSpace::GivenSecondOrder);
    for (float xi_val : xi) {
        float y_val;
        if (spline.SinglePointInterp(xi_val, y_val)) {
            after.push_back(y_val);
        } else {
            after.push_back(0.0f);
        }
    }
}

QVector<float> interp1(const QVector<float>& x, const QVector<float>& y, const QVector<float>& xi) {
    int N = x.size();
    int M = xi.size();

    QVector<float> yi(M);

    if (y.size() != N) {
        qDebug() << "interp1: x and y size mismatch";
        return yi;
    }

    SplineSpace::Spline spline(x.data(), y.data(), N, SplineSpace::GivenSecondOrder);

    for (int i = 0; i < M; ++i) {
        float x_val = xi[i];
        float y_val;
        if (spline.SinglePointInterp(x_val, y_val)) {
            yi[i] = y_val;
        } else {
            yi[i] = std::nanf("");
        }
    }

    return yi;
}

QVector<int> findPeaks(const QVector<float>& data, float minHeight) {
    QVector<int> peaks;
    for (int i = 1; i < data.size() - 1; ++i) {
        if (data[i] > data[i - 1] && data[i] > data[i + 1] && data[i] > minHeight) {
            peaks.append(i);
        }
    }
    return peaks;
}
