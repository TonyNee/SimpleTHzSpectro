#include "XYAxis.h"
#include <cmath>
#include <QtMath>
#include <QDebug>

XYAxis::XYAxis(QObject *parent)
    : QObject(parent)
{
}

/** @brief 初始化坐标轴参数
 *  @param position 坐标轴位置（左/右/上/下）
 *  @param minLimit/maxLimit 轴取值范围上下限
 *  @param minRange 最小显示范围
 *  @param minValue/maxValue 初始显示值范围
 */
void XYAxis::init(AxisPosition position, double minLimit, double maxLimit,
                  double minRange, double minValue, double maxValue)
{
    this->thePosition = position;
    this->minLimit = minLimit;
    this->maxLimit = maxLimit;
    this->minRange = minRange;
    this->minValue = minValue;
    this->maxValue = maxValue;
}

XYAxis::AxisPosition XYAxis::getAxisPosition() const
{
    return thePosition;
}

/** @brief 设置坐标轴位置并触发重绘 */
void XYAxis::setAxisPosition(AxisPosition position)
{
    if (thePosition != position)
    {
        thePosition = position;
        emit axisChanged();
    }
}

XYAxis::TickMode XYAxis::getTickMode() const
{
    return theMode;
}

/** @brief 设置刻度模式并重新计算刻度 */
void XYAxis::setTickMode(TickMode mode)
{
    if (theMode != mode)
    {
        theMode = mode;
        calcAxis();
    }
}

QRect XYAxis::getRect() const
{
    return theRect;
}

/** @brief 设置轴矩形区域并重新计算刻度 */
void XYAxis::setRect(const QRect &rect)
{
    if (theRect != rect && rect.isValid())
    {
        theRect = rect;
        calcAxis();
    }
}

int XYAxis::getDecimalPrecision() const
{
    return decimalPrecision;
}

void XYAxis::setDecimalPrecision(int precison)
{
    if (decimalPrecision != precison)
    {
        decimalPrecision = precison;
        emit axisChanged();
    }
}

double XYAxis::getFixedValueSpace() const
{
    return fixedValueSpace;
}

/** @brief 设置固定刻度间隔并重算刻度 */
void XYAxis::setFixedValueSpace(double value)
{
    fixedValueSpace = value;
    calcAxis();
}

int XYAxis::getRefPixelSpace() const
{
    return refPixelSpace;
}

/** @brief 设置参考像素间隔并重算刻度 */
void XYAxis::setRefPixelSpace(int pixel)
{
    refPixelSpace = pixel;
    calcAxis();
}

QVector<int> XYAxis::getTickPos() const
{
    return tickPos;
}

QVector<QString> XYAxis::getTickLabel() const
{
    return tickLabel;
}

double XYAxis::getMinLimit() const
{
    return minLimit;
}

double XYAxis::getMaxLimit() const
{
    return maxLimit;
}

double XYAxis::getMinRange() const
{
    return minRange;
}

double XYAxis::getMinValue() const
{
    return minValue;
}

double XYAxis::getMaxValue() const
{
    return maxValue;
}

/** @brief 获取每像素对应的值增量（像素→值的换算系数） */
double XYAxis::getUnit1PxToValue() const
{
    return unit1PxToValue;
}

/** @brief 获取每单位值对应的像素数（值→像素的换算系数） */
double XYAxis::getUnit1ValueToPx() const
{
    return unit1ValueToPx;
}

/** @brief 像素坐标转换为轴值 */
double XYAxis::pxToValue(double px) const
{
    return px * unit1PxToValue + minValue;
}

/** @brief 轴值转换为像素坐标 */
double XYAxis::valueToPx(double value) const
{
    return (value - minValue) * unit1ValueToPx;
}

/** @brief 绘制坐标轴（根据位置分发到具体绘制函数） */
void XYAxis::draw(QPainter *painter)
{
    painter->setFont(QFont("sans-serif", 11, 1));
    painter->setPen(QColor(255, 255, 255));
    switch (this->getAxisPosition())
    {
    case AtRight:
        break;
    case AtLeft:
        drawLeft(painter);
        break;
    case AtTop:
        break;
    case AtBottom:
        drawBottom(painter);
        break;
    default:
        break;
    }
}

/** @brief 绘制左侧Y轴刻度标签 */
void XYAxis::drawLeft(QPainter *painter)
{
    painter->save();

    const int right_pos = theRect.right();
    for (int i = 0; i < tickPos.count(); i++)
    {
        const int y_pos = tickPos.at(i);
        // 标签右对齐，放在轴左侧
        painter->drawText(right_pos - 25 - painter->fontMetrics().boundingRect(tickLabel.at(i)).width(),
                          y_pos + painter->fontMetrics().height() / 2,
                          tickLabel.at(i));
    }

    painter->restore();
}

/** @brief 绘制底部X轴刻度标签 */
void XYAxis::drawBottom(QPainter *painter)
{
    painter->save();

    const int top_pos = theRect.top();
    for (int i = 0; i < tickPos.count(); i++)
    {
        const int x_pos = tickPos.at(i);
        // 标签居中，放在轴下方
        painter->drawText(x_pos - painter->fontMetrics().boundingRect(tickLabel.at(i)).width() / 2,
                          top_pos + 25 + painter->fontMetrics().height(),
                          tickLabel.at(i));
    }
    painter->restore();
}

/** @brief 计算坐标轴刻度位置和标签
 *  先进行边界校验，再根据轴位置调用calcSpace计算间隔，最后生成刻度线
 */
void XYAxis::calcAxis()
{
    if (minLimit >= maxLimit || theRect.isNull())
        return;
    if (minValue > maxValue)
    {
        std::swap(minValue, maxValue);
    }
    if (minLimit > minValue)
    {
        minValue = minLimit;
    }
    if (maxLimit < maxValue)
    {
        maxValue = maxLimit;
    }
    switch (this->getAxisPosition())
    {
    case AtBottom:
    {
        // 横向X轴
        calcSpace(theRect.width() - 1);
        // 计算刻度线
        const double right_pos = theRect.right();
        tickPos.clear();
        tickLabel.clear();
        const int precision = getTickPrecision();
        // i: 刻度像素位置；j: 刻度值索引
        // 条件i<right_pos+2 确保显示最右端刻度
        for (double i = theRect.left() + pxStart, j = pxStart; i < right_pos + 2; i += pxSpace, j += pxSpace)
        {
            tickPos.push_back(std::round(i));
            const double label_value = (minValue + (j)*unit1PxToValue);
            QString label_text = QString::number(label_value, 'f', precision);
            if (label_text == "-0")
            { // 避免出现"-0"显示
                label_text = "0";
            }
            tickLabel.push_back(label_text);
        }
    }
        break;
    case AtLeft:
    {
        // 纵向Y轴
        calcSpace(theRect.height() - 1);
        // 计算刻度线
        const double top_pos = theRect.top();
        tickPos.clear();
        tickLabel.clear();
        const int precision = getTickPrecision();
        // i: 刻度像素位置（从下往上）；j: 刻度值索引
        for (double i = theRect.bottom() - pxStart, j = pxStart; i > top_pos - 2; i -= pxSpace, j += pxSpace)
        {
            tickPos.push_back(std::round(i));
            const double label_value = (minValue + (j)*unit1PxToValue);
            QString label_text = QString::number(label_value, 'f', precision);
            if (label_text == "-0")
            { // 避免出现"-0"显示
                label_text = "0";
            }
            tickLabel.push_back(label_text);
        }
    }
        break;
    default:
        break;
    }
    emit axisChanged();
}

/** @brief 计算像素-值换算系数、刻度间隔和起始偏移
 *  先算unit1PxToValue（每像素对应值）和unit1ValueToPx（每值对应像素），
 *  再根据TickMode计算valueSpace、pxSpace、pxStart
 */
void XYAxis::calcSpace(double axisLength)
{
    // 计算像素与值的互相换算系数
    unit1PxToValue = (maxValue - minValue) / (axisLength);
    unit1ValueToPx = (axisLength) / (maxValue - minValue);

    switch (theMode)
    {
    case FixedValue:
        // 固定值间隔模式：ValueSpace由外部指定
        valueSpace = fixedValueSpace;
        pxSpace = calcPxSpace(unit1PxToValue, valueSpace);
        pxStart = calcPxStart(unit1PxToValue, valueSpace, minValue, maxValue);
        break;
    case RefPixel:
        // 参考像素间隔模式：根据像素间隔反算值间隔
        valueSpace = calcValueSpace(unit1PxToValue, refPixelSpace);
        pxSpace = calcPxSpace(unit1PxToValue, valueSpace);
        pxStart = calcPxStart(unit1PxToValue, valueSpace, minValue, maxValue);
        break;
    default:
        break;
    }
}

/** @brief 根据值间隔和单位像素值计算像素间隔 */
double XYAxis::calcPxSpace(double unitP2V, double valueSpace) const
{
    if (unitP2V <= 0.0)
    {
        qWarning() << __FUNCTION__ << "unitP2V is too min" << unitP2V;
        return 30.0;
    }
    return valueSpace / unitP2V;
}

/** @brief 计算第一个刻度的起始像素偏移
 *  使刻度值对齐到valueSpace的整数倍，保证刻度从"整"值开始
 */
double XYAxis::calcPxStart(double unitP2V, double valueSpace, double valueMin, double valueMax) const
{
    Q_UNUSED(valueMax)
    if (unitP2V <= 0.0 || valueSpace <= 0.0)
    {
        qWarning() << __FUNCTION__ << "unitP2V or valueSpace is too min" << unitP2V << valueSpace;
        return 0.0;
    }
    // min有正负，而unit和space只有正
    // 从最小值往上找第一个能被value_space整除的数作为起始值
    // 如果最小值为负数且绝对值小于value_space，则起点为0
    // 即起点值应该是value_space的整倍数
    const double begin_precision = std::pow(10, decimalPrecision);
    const double begin_cut = (decimalPrecision <= 0)
            ? 0
            : qRound(std::abs(valueMin) * begin_precision) % qRound(valueSpace * begin_precision) / begin_precision;
    // 起点值不会为负，因为cut由valueSpace取模得到且均为正
    const double begin_val = qFuzzyIsNull(begin_cut) ? 0.0 : (valueMin >= 0.0) ? (valueSpace - begin_cut)
                                                                               : begin_cut;
    return begin_val / unitP2V;
}

/** @brief 根据单位像素值和参考像素间隔计算合适的值间隔
 *  使间隔值尽量"整"（如1,2,5的倍数）
 */
double XYAxis::calcValueSpace(double unitP2V, int pxRefSpace) const
{
    const double space_ref = unitP2V * pxRefSpace;
    double space_temp = space_ref;
    if (space_ref > 1)
        space_temp = calcValueSpaceHelper(space_ref, 1);
    else
        space_temp = calcValueSpaceHelper(space_ref * std::pow(10, decimalPrecision), 1) * std::pow(10, -decimalPrecision);
    return space_temp;
}

/** @brief 递归找到最合适的刻度间隔值
 *  将数值映射到1,2,4,5的倍数，使刻度可读性好
 */
double XYAxis::calcValueSpaceHelper(double valueRefRange, int dividend) const
{
    if (valueRefRange > 8 * dividend)
    {
        return calcValueSpaceHelper(valueRefRange, dividend * 10);
    }
    else if (valueRefRange > 4.5 * dividend)
    {
        return 5 * dividend;
    }
    else if (valueRefRange > 3 * dividend)
    {
        return 4 * dividend;
    }
    else if (valueRefRange > 1.5 * dividend)
    {
        return 2 * dividend;
    }
    else
    {
        return dividend;
    }
}

/** @brief 自动计算刻度标签的小数位数 */
int XYAxis::getTickPrecision() const
{
    return getTickPrecisionHelper(valueSpace, 1, 0);
}

/** @brief 递归确定小数值需要保留几位小数 */
int XYAxis::getTickPrecisionHelper(double valueSpace, double compare, int precision) const
{
    // compare从1开始每次除以10，当valueSpace大于compare时返回当前precision
    if (valueSpace >= compare)
    {
        return precision;
    }
    return getTickPrecisionHelper(valueSpace, compare / 10, precision + 1);
}

/** @brief 获取加/减步进值 */
double XYAxis::valueCalcStep() const
{
    switch (theMode)
    {
    case RefPixel:
        return valueSpace;
        break;
    case FixedValue:
        return (maxValue - minValue) / 5;
        break;
    default:
        break;
    }
    return valueSpace;
}

/** @brief 获取放大步进值（默认缩为当前范围的1/4） */
double XYAxis::valueZoomInStep() const
{
    return (maxValue - minValue) / 4;
}

/** @brief 获取缩小步进值（默认放为当前范围的1/2） */
double XYAxis::valueZoomOutStep() const
{
    return (maxValue - minValue) / 2;
}

/** @brief 根据鼠标位置计算在轴矩形内的相对比例
 *  用于按鼠标位置缩放时确定左右/上下分配比例
 */
double XYAxis::calcZoomProportionWithPos(const QPoint &pos) const
{
    double zoom_proportion = 0.5;
    switch (this->getAxisPosition())
    {
    case AtTop:
    case AtBottom:
    {
        // 横向轴：按X位置计算比例
        const int pos_x = pos.x();
        const int rect_left = theRect.left();
        const int rect_right = theRect.right();
        zoom_proportion = (pos_x - rect_left) / (double)(rect_right - rect_left);
    }
        break;
    case AtRight:
    case AtLeft:
    {
        // 纵向轴：按Y位置计算比例（注意Y轴从下往上为正）
        const int pos_y = pos.y();
        const int rect_top = theRect.top();
        const int rect_bottom = theRect.bottom();
        zoom_proportion = (rect_bottom - pos_y) / (double)(rect_bottom - rect_top);
    }
        break;
    default:
        break;
    }
    if (zoom_proportion <= 0.0)
        return 0.0;
    if (zoom_proportion >= 1.0)
        return 1.0;
    return zoom_proportion;
}

/** @brief 增大最小值（向上平移） */
void XYAxis::addMinValue()
{
    if (maxValue - minValue <= minRange)
        return;
    minValue += valueCalcStep();
    if (maxValue - minValue < minRange)
    {
        minValue = maxValue - minRange;
    }
    calcAxis();
}

/** @brief 减小最小值（向下平移） */
void XYAxis::subMinValue()
{
    if (minValue <= minLimit)
        return;
    minValue -= valueCalcStep();
    if (minValue < minLimit)
    {
        minValue = minLimit;
    }
    calcAxis();
}

/** @brief 增大最大值（向上扩展） */
void XYAxis::addMaxValue()
{
    if (maxValue > maxLimit)
        return;
    maxValue += valueCalcStep();
    if (maxValue > maxLimit)
    {
        maxValue = maxLimit;
    }
    calcAxis();
}

/** @brief 减小最大值（向下收缩） */
void XYAxis::subMaxValue()
{
    if (maxValue - minValue <= minRange)
        return;
    maxValue -= valueCalcStep();
    if (maxValue - minValue < minRange)
    {
        maxValue = minValue + minRange;
    }
    calcAxis();
}

/** @brief 按像素量平移坐标轴
 *  @param px 正数向右/上移动，负数向左/下移动
 *  @return 是否实际发生了移动
 */
bool XYAxis::moveValueWidthPx(int px)
{
    double move_step = qAbs(px) * unit1PxToValue;
    if (move_step <= 0)
        return false;
    // px<0: 向min端移动（左/下）；px>0: 向max端移动（右/上）
    if (px < 0)
    {
        if (minValue <= minLimit)
            return false;
        if (minValue - move_step < minLimit)
        {
            move_step = minValue - minLimit;
        }
        minValue -= move_step;
        maxValue -= move_step;
    }
    else
    {
        if (maxValue > maxLimit)
            return false;
        if (maxValue + move_step > maxLimit)
        {
            move_step = maxLimit - maxValue;
        }
        minValue += move_step;
        maxValue += move_step;
    }
    calcAxis();
    return true;
}

/** @brief 放大（缩小显示范围），以中心为基准 */
void XYAxis::zoomValueIn()
{
    const double val_range = maxValue - minValue;
    if (val_range <= minRange)
        return;
    const double zoom_step = valueZoomInStep();
    if (zoom_step <= 0)
        return;
    if (val_range - zoom_step < minRange)
    {
        const double zoom_real_step = val_range - minRange;
        minValue += zoom_real_step / 2;
        maxValue = minValue + minRange;
    }
    else
    {
        minValue += zoom_step / 2;
        maxValue -= zoom_step / 2;
    }

    calcAxis();
}

/** @brief 缩小（扩大显示范围），以中心为基准 */
void XYAxis::zoomValueOut()
{
    if (minValue <= minLimit && maxValue >= maxLimit)
        return;
    const double zoom_half = valueZoomOutStep() / 2;
    // 边界保护：不能超出limit限制
    const double min_zoom = (minValue - zoom_half < minLimit)
            ? (minValue - minLimit)
            : (zoom_half);
    const double max_zoom = (maxValue + zoom_half > maxLimit)
            ? (maxLimit - maxValue)
            : (zoom_half);
    minValue -= min_zoom;
    maxValue += max_zoom;

    calcAxis();
}

/** @brief 按鼠标位置放大（鼠标所在位置保持不动） */
void XYAxis::zoomValueInPos(const QPoint &pos)
{
    const double val_range = maxValue - minValue;
    if (val_range <= minRange)
        return;
    const double zoom_step = valueZoomInStep();
    if (zoom_step <= 0)
        return;
    const double zoom_proportion = calcZoomProportionWithPos(pos);
    if (val_range - zoom_step < minRange)
    {
        const double zoom_real_step = val_range - minRange;
        minValue += zoom_real_step / 2;
        maxValue = minValue + minRange;
    }
    else
    {
        // 按比例分配缩放量，鼠标处保持不变
        minValue += zoom_step * zoom_proportion;
        maxValue -= zoom_step * (1 - zoom_proportion);
    }

    calcAxis();
}

/** @brief 按鼠标位置缩小（鼠标所在位置保持不动） */
void XYAxis::zoomValueOutPos(const QPoint &pos)
{
    if (minValue <= minLimit && maxValue >= maxLimit)
        return;
    const double zoom_proportion = calcZoomProportionWithPos(pos);
    const double zoom_step = valueZoomInStep();
    const double min_step = zoom_step * zoom_proportion;
    const double max_step = zoom_step * (1 - zoom_proportion);
    const double min_zoom = (minValue - min_step < minLimit)
            ? (minValue - minLimit)
            : (min_step);
    const double max_zoom = (maxValue + max_step > maxLimit)
            ? (maxLimit - maxValue)
            : (max_step);
    minValue -= min_zoom;
    maxValue += max_zoom;

    calcAxis();
}

/** @brief 恢复全景视图（将坐标轴设置为limit全量程） */
void XYAxis::overallView()
{
    if (minValue <= minLimit && maxValue >= maxLimit)
        return;
    minValue = minLimit;
    maxValue = maxLimit;
    calcAxis();
}

/** @brief 设置轴的量程限制范围 */
void XYAxis::setLimitRange(double min, double max, double range)
{
    if (min >= max || max - min < range)
        return;
    minLimit = min;
    maxLimit = max;
    minRange = range;
    emit axisChanged();
}

/** @brief 设置轴的当前显示值范围 */
void XYAxis::setValueRange(double min, double max)
{
    if (min >= max || max - min <= minRange)
        return;
    minValue = min;
    maxValue = max;
    calcAxis();
}
