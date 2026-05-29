#include "XYView.h"
#include <cmath>
#include <QtMath>
#include <QGuiApplication>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>

XYView::XYView(QWidget *parent) : QWidget(parent)
{
    setMouseTracking(true);
    xAxis = new XYAxis(this);
    xAxis->init(XYAxis::AtBottom, defaultXMin, defaultXMax, 10, defaultXMin, defaultXMax);
    yAxis = new XYAxis(this);
    yAxis->init(XYAxis::AtLeft, defaultYMin, defaultYMax, 0.001, defaultYMin, defaultYMax);

    connect(xAxis, &XYAxis::axisChanged, this, &XYView::refresh);
    connect(yAxis, &XYAxis::axisChanged, this, &XYView::refresh);
}

void XYView::paintEvent(QPaintEvent *event)
{
    event->accept();
    QPainter painter(this);

    painter.fillRect(plotArea, QColor(0, 0, 0));
    painter.setFont(QFont("sans-serif", 13, 1));

    // 绘制网格
    painter.setPen(QColor(120, 120, 120));
    auto x_tick = xAxis->getTickPos();
    auto y_tick = yAxis->getTickPos();
    for (int px : x_tick)
    {
        if (px >= plotArea.left() && px <= plotArea.right())
        {
            painter.drawLine(px, plotArea.top(), px, plotArea.bottom());
        }
    }
    for (int px : y_tick)
    {
        if (px >= plotArea.top() && px <= plotArea.bottom())
        {
            painter.drawLine(plotArea.left(), px, plotArea.right(), px);
        }
    }

    // 绘制坐标轴
    painter.setPen(QColor(255, 0, 0, 100));
    xAxis->draw(&painter);
    yAxis->draw(&painter);

    // 绘制曲线
    int mouse_index = -1;
    const int xBase = xAxis->getRect().left();  // 曲线与X轴刻度对齐
    QPoint plot_pos = mousePos - plotArea.topLeft();
    if (!seriesData.isEmpty())
    {
        QPainterPath path;
        path.moveTo(xBase + xAxis->valueToPx(seriesData.first().x),
                    plotArea.bottom() - yAxis->valueToPx(seriesData.first().y));
        for (const Node &item : seriesData)
        {
            path.lineTo(xBase + xAxis->valueToPx(item.x),
                        plotArea.bottom() - yAxis->valueToPx(item.y));
        }

        painter.save();
        painter.setClipRect(plotArea);
        painter.setPen(QColor(0, 220, 0));
        painter.drawPath(path);

        // 鼠标X轴对应的数据点高亮
        if (plotArea.isValid() && plotArea.contains(mousePos))
        {
            const double mouse_val = xAxis->pxToValue(mousePos.x() - xBase);
            mouse_index = searchDataIndex(0, seriesData.size(), mouse_val);
            painter.setPen(QPen(QColor(250, 0, 0, 200), 6));
            painter.drawPoint(xBase + xAxis->valueToPx(seriesData.at(mouse_index).x),
                              plotArea.bottom() - yAxis->valueToPx(seriesData.at(mouse_index).y));
        }
        painter.restore();
    }

    // 绘制光标十字线
    if (plotArea.isValid() && plotArea.contains(mousePos))
    {
        painter.setPen(QColor(255, 180, 0));
        painter.drawLine(mousePos.x(), plotArea.top(), mousePos.x(), plotArea.bottom());
        painter.drawLine(plotArea.left(), mousePos.y(), plotArea.right(), mousePos.y());
        QString val = QString("(%1, %2)")
                .arg(QString::number(xAxis->pxToValue(mousePos.x() - xBase), 'f', 2))
                .arg(QString::number(yAxis->pxToValue(plotArea.bottom() - plot_pos.y()), 'f', 4));
        if (mouse_index >= 0)
        {
            val = QString("(%1, %2) ")
                    .arg(QString::number(seriesData.at(mouse_index).x, 'f', 2))
                    .arg(QString::number(seriesData.at(mouse_index).y, 'f', 4)) + val;
        }
        painter.drawText(mousePos + QPoint(10, -10), val);
    }
}

void XYView::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    int margin = 10;
    contentArea = rect().adjusted(margin, margin, -margin, -margin);
    xAxis->setRect(QRect(contentArea.left() + 50, contentArea.bottom() - 50,
                         contentArea.width() - 50, 50 + 1));
    yAxis->setRect(QRect(contentArea.left(), contentArea.top(),
                         60 + 1, contentArea.height() - 50));
    plotArea = contentArea.adjusted(0, -50, 50, 0);
}

void XYView::mousePressEvent(QMouseEvent *event)
{
    event->accept();
    mousePos = event->pos();
    prevPos = mousePos;
    if (event->button() == Qt::LeftButton)
    {
        pressFlag = true;
    }
    refresh();
}

void XYView::mouseMoveEvent(QMouseEvent *event)
{
    event->accept();
    mousePos = event->pos();
    if (pressFlag)
    {
        xAxis->moveValueWidthPx(prevPos.x() - mousePos.x());
        yAxis->moveValueWidthPx(mousePos.y() - prevPos.y());
    }
    prevPos = mousePos;
    refresh();
}

void XYView::mouseReleaseEvent(QMouseEvent *event)
{
    event->accept();
    mousePos = event->pos();
    pressFlag = false;
    refresh();
}

void XYView::leaveEvent(QEvent *event)
{
    QWidget::leaveEvent(event);
    mousePos = QPoint(-1, -1);
    refresh();
}

void XYView::wheelEvent(QWheelEvent *event)
{
    event->accept();
#if (QT_VERSION <= QT_VERSION_CHECK(5, 15, 0))
    const QPoint pos = event->pos();
    const int delta = event->delta();
#else
    const QPoint pos = event->position().toPoint();
    const int delta = event->angleDelta().y();
#endif
    const Qt::KeyboardModifiers key_mod = QGuiApplication::keyboardModifiers();
    const bool delta_up = (delta > 0);
    if (key_mod & Qt::ControlModifier)
    {
        if (delta_up)
            yAxis->zoomValueInPos(pos);
        else
            yAxis->zoomValueOutPos(pos);
    }
    else
    {
        if (delta_up)
            xAxis->zoomValueInPos(pos);
        else
            xAxis->zoomValueOutPos(pos);
    }
}

int XYView::searchDataIndex(int start, int end, double distinction) const
{
    if (distinction >= seriesData.at(end - 1).x)
        return end - 1;
    if (distinction <= seriesData.at(start).x)
        return start;
    int mid;
    while (true)
    {
        mid = (start + end) / 2;
        if (mid > 0 && seriesData.at(mid - 1).x < distinction && seriesData.at(mid).x > distinction)
        {
            return ((seriesData.at(mid).x - distinction) >= (distinction - seriesData.at(mid - 1).x))
                    ? mid - 1
                    : mid;
        }
        if (distinction > seriesData.at(mid).x)
            start = mid;
        else if (distinction < seriesData.at(mid).x)
            end = mid;
        else
            return mid;
    }
}

void XYView::refresh()
{
    update();
}

void XYView::setdata(QVector<std::pair<float, float>>& data)
{
    seriesData.clear();
    for (const auto &item : data) {
        if (item.first < 200) {
            continue;
        }
        seriesData.append(Node{item.first, item.second * 1.5f + 0.16f});
    }
    update();
}

void XYView::setAxisLabel(const QString& xLabel, const QString& yLabel)
{
    xAxisLabel = xLabel;
    yAxisLabel = yLabel;
}
