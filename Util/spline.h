#ifndef SPLINE_H
#define SPLINE_H

namespace SplineSpace
{
    enum BoundaryCondition
    {
        GivenFirstOrder = 1,    // 第一类边界条件：给定端点一阶导数
        GivenSecondOrder = 2    // 第二类边界条件：给定端点二阶导数（默认自然边界为0）
    };

    class Spline
    {
    public:
        // 三点以上已知点构造样条；默认使用自然边界（二阶导为0）
        Spline(const float* x0, const float* y0, const int& num,
               BoundaryCondition bc = GivenSecondOrder,
               const float& leftBoundary = 0.0f, const float& rightBoundary = 0.0f);
        ~Spline();

        // 单个点插值；成功返回true，x超出范围返回false
        bool SinglePointInterp(const float& x, float& y) noexcept(false);
        // 批量插值
        bool MultiPointInterp(const float* x, const int& num, float* y) noexcept(false);

    private:
        void PartialDerivative1(void); // 第一类边界求偏导
        void PartialDerivative2(void); // 第二类边界求偏导

    private:
        const float* GivenX;
        const float* GivenY;
        const int GivenNum;
        float* PartialDerivative;
        BoundaryCondition Bc;
        float LeftB;
        float RightB;
        float MaxX;
        float MinX;
    };
}

#endif // SPLINE_H
