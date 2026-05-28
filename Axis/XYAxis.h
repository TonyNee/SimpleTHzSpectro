#pragma once
#include <QObject>
#include <QPainter>

class XYAxis : public QObject
{
    Q_OBJECT
public:
    enum AxisPosition
    {
        AtLeft,
        AtRight,
        AtTop,
        AtBottom
    };
    enum TickMode
    {
        FixedValue,
        RefPixel
    };

public:
    explicit XYAxis(QObject *parent = nullptr);
    void init(AxisPosition position, double minLimit, double maxLimit,
              double minRange, double minValue, double maxValue);

    AxisPosition getAxisPosition() const;
    void setAxisPosition(AxisPosition position);

    TickMode getTickMode() const;
    void setTickMode(TickMode mode);

    QRect getRect() const;
    void setRect(const QRect &rect);

    int getDecimalPrecision() const;
    void setDecimalPrecision(int precison);

    double getFixedValueSpace() const;
    void setFixedValueSpace(double value);

    int getRefPixelSpace() const;
    void setRefPixelSpace(int pixel);

    QVector<int> getTickPos() const;
    QVector<QString> getTickLabel() const;

    double getMinLimit() const;
    double getMaxLimit() const;
    double getMinRange() const;
    double getMinValue() const;
    double getMaxValue() const;

    double getUnit1PxToValue() const;
    double getUnit1ValueToPx() const;

    double pxToValue(double px) const;
    double valueToPx(double value) const;

    void draw(QPainter *painter);

private:
    void drawLeft(QPainter *painter);
    void drawBottom(QPainter *painter);
    void drawTop(QPainter *painter);
    void drawRight(QPainter *painter);
    void calcAxis();
    void calcSpace(double axisLength);
    double calcPxSpace(double unitP2V, double valueSpace) const;
    double calcPxStart(double unitP2V, double valueSpace, double valueMin, double valueMax) const;
    double calcValueSpace(double unitP2V, int pxRefSpace) const;
    double calcValueSpaceHelper(double valueRefRange, int dividend) const;
    int getTickPrecision() const;
    int getTickPrecisionHelper(double valueSpace, double compare, int precision) const;
    double valueCalcStep() const;
    double valueZoomInStep() const;
    double valueZoomOutStep() const;
    double calcZoomProportionWithPos(const QPoint &pos) const;

signals:
    void axisChanged();

public slots:
    void addMinValue();
    void subMinValue();
    void addMaxValue();
    void subMaxValue();
    bool moveValueWidthPx(int px);
    void zoomValueIn();
    void zoomValueOut();
    void zoomValueInPos(const QPoint &pos);
    void zoomValueOutPos(const QPoint &pos);
    void overallView();
    void setLimitRange(double min, double max, double range);
    void setValueRange(double min, double max);

private:
    AxisPosition thePosition{AtLeft};
    TickMode theMode{RefPixel};
    QRect theRect;
    int decimalPrecision{3};
    double fixedValueSpace{100.0};
    double refPixelSpace{80.0};
    QVector<int> tickPos;
    QVector<QString> tickLabel;

    double minLimit{0.0};
    double maxLimit{1000.0};
    double minRange{10.0};
    double minValue{0.0};
    double maxValue{1000.0};

    double unit1PxToValue{1.0};
    double unit1ValueToPx{1.0};
    double pxStart{0.0};
    double pxSpace{30.0};
    double valueSpace{1.0};
};
