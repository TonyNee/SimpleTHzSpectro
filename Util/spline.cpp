#include "spline.h"
#include <iostream>
#include <algorithm>

namespace SplineSpace
{
    Spline::Spline(const float* x0, const float* y0, const int& num,
                   BoundaryCondition bc, const float& leftBoundary, const float& rightBoundary)
            : GivenX(x0), GivenY(y0), GivenNum(num), Bc(bc), LeftB(leftBoundary), RightB(rightBoundary)
    {
        if ((x0 == NULL) | (y0 == NULL) | (num < 3))
        {
            std::cout << "Spline construction failed: too few points" << std::endl;
        }
        PartialDerivative = new float[GivenNum];
        MaxX = *std::max_element(GivenX, GivenX + GivenNum);
        MinX = *std::min_element(GivenX, GivenX + GivenNum);
        if (Bc == GivenFirstOrder)
            PartialDerivative1();
        else if (Bc == GivenSecondOrder)
            PartialDerivative2();
        else
        {
            delete[] PartialDerivative;
            std::cout << "Boundary condition error" << std::endl;
        }
    }

    void Spline::PartialDerivative1(void)
    {
        float *a = new float[GivenNum];
        float *b = new float[GivenNum];
        float *c = new float[GivenNum];
        float *d = new float[GivenNum];
        float *f = new float[GivenNum];
        float *bt = new float[GivenNum];
        float *gm = new float[GivenNum];
        float *h = new float[GivenNum];

        for (int i = 0; i < GivenNum; i++) b[i] = 2;
        for (int i = 0; i < GivenNum - 1; i++) h[i] = GivenX[i + 1] - GivenX[i];
        for (int i = 1; i < GivenNum - 1; i++) a[i] = h[i - 1] / (h[i - 1] + h[i]);
        a[GivenNum - 1] = 1;

        c[0] = 1;
        for (int i = 1; i < GivenNum - 1; i++) c[i] = h[i] / (h[i - 1] + h[i]);

        for (int i = 0; i < GivenNum - 1; i++)
            f[i] = (GivenY[i + 1] - GivenY[i]) / (GivenX[i + 1] - GivenX[i]);

        d[0] = 6 * (f[0] - LeftB) / h[0];
        d[GivenNum - 1] = 6 * (RightB - f[GivenNum - 2]) / h[GivenNum - 2];

        for (int i = 1; i < GivenNum - 1; i++) d[i] = 6 * (f[i] - f[i - 1]) / (h[i - 1] + h[i]);

        bt[0] = c[0] / b[0];
        for (int i = 1; i < GivenNum - 1; i++) bt[i] = c[i] / (b[i] - a[i] * bt[i - 1]);

        gm[0] = d[0] / b[0];
        for (int i = 1; i <= GivenNum - 1; i++) gm[i] = (d[i] - a[i] * gm[i - 1]) / (b[i] - a[i] * bt[i - 1]);

        PartialDerivative[GivenNum - 1] = gm[GivenNum - 1];
        for (int i = GivenNum - 2; i >= 0; i--) PartialDerivative[i] = gm[i] - bt[i] * PartialDerivative[i + 1];

        delete[] a; delete[] b; delete[] c; delete[] d;
        delete[] gm; delete[] bt; delete[] f; delete[] h;
    }

    void Spline::PartialDerivative2(void)
    {
        float *a = new float[GivenNum];
        float *b = new float[GivenNum];
        float *c = new float[GivenNum];
        float *d = new float[GivenNum];
        float *f = new float[GivenNum];
        float *bt = new float[GivenNum];
        float *gm = new float[GivenNum];
        float *h = new float[GivenNum];

        for (int i = 0; i < GivenNum; i++) b[i] = 2;
        for (int i = 0; i < GivenNum - 1; i++) h[i] = GivenX[i + 1] - GivenX[i];
        for (int i = 1; i < GivenNum - 1; i++) a[i] = h[i - 1] / (h[i - 1] + h[i]);
        a[GivenNum - 1] = 1;

        c[0] = 1;
        for (int i = 1; i < GivenNum - 1; i++) c[i] = h[i] / (h[i - 1] + h[i]);

        for (int i = 0; i < GivenNum - 1; i++)
            f[i] = (GivenY[i + 1] - GivenY[i]) / (GivenX[i + 1] - GivenX[i]);

        for (int i = 1; i < GivenNum - 1; i++) d[i] = 6 * (f[i] - f[i - 1]) / (h[i - 1] + h[i]);

        d[1] = d[1] - a[1] * LeftB;
        d[GivenNum - 2] = d[GivenNum - 2] - c[GivenNum - 2] * RightB;

        bt[1] = c[1] / b[1];
        for (int i = 2; i < GivenNum - 2; i++) bt[i] = c[i] / (b[i] - a[i] * bt[i - 1]);

        gm[1] = d[1] / b[1];
        for (int i = 2; i <= GivenNum - 2; i++) gm[i] = (d[i] - a[i] * gm[i - 1]) / (b[i] - a[i] * bt[i - 1]);

        PartialDerivative[GivenNum - 2] = gm[GivenNum - 2];
        for (int i = GivenNum - 3; i >= 1; i--) PartialDerivative[i] = gm[i] - bt[i] * PartialDerivative[i + 1];

        PartialDerivative[0] = LeftB;
        PartialDerivative[GivenNum - 1] = RightB;

        delete[] a; delete[] b; delete[] c; delete[] d;
        delete[] gm; delete[] bt; delete[] f; delete[] h;
    }

    bool Spline::SinglePointInterp(const float& x, float& y) noexcept(false)
    {
        if ((x < MinX) || (x > MaxX)) {
            std::cout << "Extrapolation not supported" << std::endl;
            return false;
        }
        int klo, khi, k;
        klo = 0; khi = GivenNum - 1;
        float hh, bb, aa;

        while (khi - klo > 1)
        {
            k = (khi + klo) >> 1;
            if (GivenX[k] > x) khi = k;
            else klo = k;
        }
        hh = GivenX[khi] - GivenX[klo];

        aa = (GivenX[khi] - x) / hh;
        bb = (x - GivenX[klo]) / hh;

        y = aa * GivenY[klo] + bb * GivenY[khi] + ((aa * aa * aa - aa) * PartialDerivative[klo] + (bb * bb * bb - bb) * PartialDerivative[khi]) * hh * hh / 6.0f;
        return true;
    }

    bool Spline::MultiPointInterp(const float* x, const int& num, float* y) noexcept(false)
    {
        for (int i = 0; i < num; i++)
        {
            SinglePointInterp(x[i], y[i]);
        }
        return true;
    }

    Spline::~Spline()
    {
        delete[] PartialDerivative;
    }
}
