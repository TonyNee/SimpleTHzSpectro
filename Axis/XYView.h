#pragma once
#include <QWidget>
#include <utility>
#include "XYAxis.h"

/**
 * @brief 笛卡尔坐标系图表绘制组件（单通道波形显示）
 *
 * 支持鼠标拖拽平移、滚轮缩放、十字光标读数。
 * 内部使用XYAxis管理坐标轴刻度计算与渲染。
 */
class XYView : public QWidget
{
    Q_OBJECT
public:
    explicit XYView(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    int searchDataIndex(int start, int end, double distinction) const;

public slots:
    void setdata(QVector<std::pair<float, float>>& data);
    void setAxisLabel(const QString& xLabel, const QString& yLabel);
    void refresh();

private:
    XYAxis *xAxis;
    XYAxis *yAxis;
    QRect contentArea;
    QRect plotArea;
    QPoint mousePos;
    QPoint prevPos;
    bool pressFlag{false};

    double defaultXMin = 200;
    double defaultXMax = 910;
    double defaultYMin = -0.5;
    double defaultYMax = 1.6;

    QString xAxisLabel;
    QString yAxisLabel;

    struct Node { float x; float y; };
    QVector<Node> seriesData;
};
