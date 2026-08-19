#include "mwaveview.h"

static QColor chColor[20] = {
    QColor(255, 228, 16),    // Yellow (AI0)
    QColor(0, 225, 255),     // Light Blue (AI1)
    QColor(0, 139, 139),    // Dark Cyan (AI2)
    QColor(0, 23, 255),      // Blue (AI3)
    QColor(127, 255, 0),     // Light Green (AI4)
    QColor(255, 0, 255),     // Magenta (AI5)
    QColor(147, 112, 219),   // Lavender (AI6)
    QColor(70, 130, 180),       // Steel Blue (AI7)
    QColor(255, 99, 71),     // Tomato (AI8)
    QColor(255, 165, 0),     // Orange (AI9)
    QColor(255, 105, 180),   // Hot Pink (AI10)
    QColor(255, 228, 16),    // Yellow (AO)
    QColor(255, 0, 0),       // Red (DI0)
    QColor(255, 228, 16),    // Yellow (DI1)
    QColor(0, 225, 255),     // Light Blue (DI2)
    QColor(255, 20, 255),    // Pink (DI3)
    QColor(0, 23, 255),      // Blue (DI4)
    QColor(255, 165, 0),     // Orange (DI5)
    QColor(147, 112, 219),   // Lavender (DI6)
    QColor(0, 255, 0),       // Green (DI7)
};

MWaveView::MWaveView(QWidget *parent) : QChartView(parent)
{
    this->wave_layout = new QBoxLayout(QBoxLayout::LeftToRight, parent);

    this->m_wave.axisX = new QValueAxis;
    this->m_wave.axisY = new QValueAxis;
    this->m_wave.axisX->setTickCount(9);
    this->m_wave.axisY->setTickCount(9);
    this->m_wave.axisX->setLabelFormat("%d");
    this->m_wave.axisY->setLabelFormat("%d");
    this->m_wave.axisX->setTickType(QValueAxis::TickType::TicksFixed);
    this->m_wave.axisX->setRange(1, 300);
    this->m_wave.axisY->setRange(-10, 10);

    this->m_wave.chart = new QChart;
    this->m_wave.chart->setBackgroundBrush(QBrush(QColor(30, 30, 30)));
    this->m_wave.chart->setTheme(QChart::ChartThemeDark);
    this->setChart(this->m_wave.chart);

    QFont legendFont;
    legendFont.setPointSize(12);
    legendFont.setBold(true);
    this->m_wave.chart->legend()->setVisible(true);
    this->m_wave.chart->legend()->setAlignment(Qt::AlignRight);
    this->m_wave.chart->legend()->setFont(legendFont);

    // 延迟移除 AO 图例项
    QTimer::singleShot(0, this, [this]() {
        if (m_wave.chart->legend()) {
            const auto markers = m_wave.chart->legend()->markers();
            for (QLegendMarker* marker : markers) {
                if (marker->series()->name() == "AO") {
                    marker->setVisible(false);  // 隐藏 AO 图例
                }
            }
        }
    });

    this->m_wave.chart->addAxis(this->m_wave.axisX, Qt::AlignBottom);
    this->m_wave.chart->addAxis(this->m_wave.axisY, Qt::AlignLeft);

    this->setRenderHint(QPainter::Antialiasing, true);

    for (int i = 0; i < SET_GLOBLE_CHANNEL; i++) {
        addChannel((WAVE_CH)i);
    }

    m_wave.rangeX = 300;
    m_wave.rangeY = 10;
    this->m_wave.multipleX = 1.2;
    this->m_wave.multipleY = 1.2;
    this->m_event.rightButtonPressed = false;
    this->m_event.pauseWave = false;

    this->m_event.menu = new QMenu(this);
    this->m_event.startAction = new QAction("\u5f00\u59cb", this);
    this->m_event.pauseAction = new QAction("\u6682\u505c", this);
    this->m_event.startAction->setVisible(false);

    this->m_event.menu->addAction(this->m_event.startAction);
    this->m_event.menu->addAction(this->m_event.pauseAction);

    connect(m_event.startAction, &QAction::triggered, [=] {
        m_event.startAction->setVisible(false);
        m_event.pauseAction->setVisible(true);
    });
    connect(m_event.pauseAction, &QAction::triggered, [=] {
        m_event.pauseAction->setVisible(false);
        m_event.startAction->setVisible(true);
    });
    connect(m_event.pauseAction, &QAction::triggered, this, &MWaveView::slots_pauseGraph);
    connect(m_event.startAction, &QAction::triggered, this, &MWaveView::slots_startGraph);
}

MWaveView::~MWaveView() {}

void MWaveView::addChannel(WAVE_CH ch)
{
    SeriesType *series = new SeriesType();
    QPen pen;
    pen.setWidth(2);
    pen.setColor(chColor[ch]);
    series->setPen(pen);
    series->setUseOpenGL(true);

    // 设置图例名称
    if (ch >= WAVE_CH12 && ch <= WAVE_CH19) {
        series->setName(QString("DI%1").arg(ch - WAVE_CH12));  // DI0~DI7
    } else if (ch >= WAVE_CH0 && ch <= WAVE_CH10) {
        series->setName(QString("AI%1").arg(ch));              // AI0~AI10
    } else if (ch == WAVE_CH11) {
        series->setName("AO");  // 模拟输出，后续手动移除图例项
    }

    this->m_wave.map_series.insert(ch, series);
    this->m_wave.chart->addSeries(series);
    series->attachAxis(m_wave.axisX);
    series->attachAxis(m_wave.axisY);
}

void MWaveView::openChannel(WAVE_CH ch) {
    if (this->m_wave.map_series.contains(ch)) {
        this->m_wave.map_series[ch]->setVisible(true);
    }
}

void MWaveView::closeChannel(WAVE_CH ch) {
    if (this->m_wave.map_series.contains(ch)) {
        this->m_wave.map_series[ch]->setVisible(false);
    }
}

void MWaveView::clearChannel(WAVE_CH ch) {
    if (this->m_wave.map_series.contains(ch)) {
        this->m_wave.map_series[ch]->clear();
    }
}

void MWaveView::setRangeX(int rangeX) { this->m_wave.rangeX = rangeX; }
void MWaveView::setRangeY(int rangeY) { this->m_wave.rangeY = rangeY; }

void MWaveView::updateRange() {
    double maxX = m_wave.last_point_x > m_wave.rangeX ? m_wave.last_point_x : m_wave.rangeX;
    double minX = maxX - m_wave.rangeX > 0 ? maxX - m_wave.rangeX : 0;

    double minY, maxY;

    if (m_wave.autoScaleY) {
        // 遍历所有可见通道，找全局 min/max
        double dataMin =  1e9, dataMax = -1e9;
        bool hasData = false;
        for (auto it = m_wave.map_series.begin(); it != m_wave.map_series.end(); ++it) {
            SeriesType *s = it.value();
            if (!s->isVisible()) continue;
            const auto pts = s->points();
            for (const QPointF &p : pts) {
                if (p.x() < minX || p.x() > maxX) continue;
                if (p.y() < dataMin) dataMin = p.y();
                if (p.y() > dataMax) dataMax = p.y();
                hasData = true;
            }
        }
        if (!hasData) {
            dataMin = -10.0;
            dataMax =  10.0;
        }
        // 留 10% 余量，至少保证 0.5V 范围避免轴塌缩
        double margin = qMax((dataMax - dataMin) * 0.1, 0.25);
        minY = dataMin - margin;
        maxY = dataMax + margin;
        m_wave.rangeY = maxY - minY;
        m_event.moveY = (maxY + minY) / 2.0;
    } else {
        maxY = m_event.moveY + m_wave.rangeY * 0.5;
        minY = m_event.moveY - m_wave.rangeY * 0.5;
    }

    int tickCount = qMax(2, static_cast<int>(maxY - minY) + 1);
    m_wave.axisY->setTickType(QValueAxis::TicksFixed);
    m_wave.axisY->setTickCount(tickCount);
    m_wave.axisY->setLabelFormat("%.1f");
    m_wave.axisY->setRange(minY, maxY);
    m_wave.axisX->setRange(minX, maxX);
}

void MWaveView::setZoomY(double multiple) {
    if (multiple > 0) {
        this->m_wave.multipleY = multiple;
    }
}

void MWaveView::ZoomOutX(void) { this->m_wave.rangeX *= 1.0 / this->m_wave.multipleX; }
void MWaveView::ZoomX(void) { this->m_wave.rangeX *= this->m_wave.multipleX; }
void MWaveView::ZoomOutY(void) { this->m_wave.rangeY *= 1.0 / this->m_wave.multipleY; }
void MWaveView::ZoomY(void) { this->m_wave.rangeY *= this->m_wave.multipleY; }

void MWaveView::startGraph(void) { this->m_event.pauseWave = false; }
void MWaveView::pauseGraph(void) { this->m_event.pauseWave = true; }
void MWaveView::slots_startGraph(void) { startGraph(); }
void MWaveView::slots_pauseGraph(void) { pauseGraph(); }

void MWaveView::addSeriesData(WAVE_CH ch, const QPointF &point) {
    if (this->m_event.pauseWave == true) return;
    if (this->m_wave.map_series.contains(ch)) {
        this->m_wave.map_series[ch]->append(point);
        if (point.x() > m_wave.last_point_x)
            m_wave.last_point_x = point.x();
        this->updateRange();
    }
}

void MWaveView::addSeriesData(WAVE_CH ch, const QList<QPointF> &point_list) {
    if (this->m_event.pauseWave == true) return;
    if (this->m_wave.map_series.contains(ch)) {
        this->m_wave.map_series[ch]->replace(point_list);
        if (!point_list.isEmpty()) {
            double x = point_list.last().x();
            if (x > m_wave.last_point_x) m_wave.last_point_x = x;
        } else {
            m_wave.last_point_x = 0;
        }
        // 不在此处调用 updateRange()，由调用方批量完成后调用 flushRange()
    }
}

void MWaveView::flushRange() {
    if (!m_event.pauseWave)
        updateRange();
}

void MWaveView::mousePressEvent(QMouseEvent *pEvent) {
    if (pEvent->button() == Qt::LeftButton) {
        this->m_event.leftButtonPressed = true;
        this->m_event.PressedPos = pEvent->pos();
        this->setCursor(Qt::OpenHandCursor);
    }
}

void MWaveView::mouseReleaseEvent(QMouseEvent *pEvent) {
    if (pEvent->button() == Qt::LeftButton) {
        this->m_event.leftButtonPressed = false;
        this->setCursor(Qt::ArrowCursor);
    }
}

void MWaveView::mouseMoveEvent(QMouseEvent *pEvent) {
    if (m_event.leftButtonPressed) {
        QPoint oDeltaPos = pEvent->pos() - m_event.PressedPos;
        m_wave.chart->scroll(-oDeltaPos.x(), oDeltaPos.y());
        QPointF pos = m_wave.chart->mapToValue(pEvent->pos()) - m_wave.chart->mapToValue(m_event.PressedPos);
        m_event.moveX += -pos.x();
        m_event.moveY += -pos.y();
        m_event.PressedPos = pEvent->pos();
    }
    QPointF f = m_wave.chart->mapToValue(pEvent->pos());
    QString str = '(' + QString::number(f.x()) + ',' + QString::number(f.y()) + ')';
    QToolTip::showText(pEvent->globalPos(), str);
}

void MWaveView::wheelEvent(QWheelEvent *pEvent) {
    double multipleX, multipleY;
    QRectF oPlotAreaRect = m_wave.chart->plotArea();
    QPointF oCenterPoint = oPlotAreaRect.center();

    if (pEvent->angleDelta().y() < 0) {
        multipleX = this->m_wave.multipleX;
        multipleY = this->m_wave.multipleY;
        ZoomX(); ZoomY();
    } else {
        multipleX = 1 / this->m_wave.multipleX;
        multipleY = 1 / this->m_wave.multipleY;
        ZoomOutX(); ZoomOutY();
    }

    oPlotAreaRect.setWidth(oPlotAreaRect.width() * multipleX);
    oPlotAreaRect.setHeight(oPlotAreaRect.height() * multipleY);
    QPointF oNewCenterPoint((2 * oCenterPoint - pEvent->position()) - (oCenterPoint - pEvent->position()) / ((multipleX + multipleY) * 0.5));
    oPlotAreaRect.moveCenter(oNewCenterPoint);
    QPointF movepos = m_wave.chart->mapToValue(oNewCenterPoint) - m_wave.chart->mapToValue(oCenterPoint);
    m_event.moveY += movepos.ry();
    this->m_wave.chart->zoomIn(oPlotAreaRect);
}

void MWaveView::contextMenuEvent(QContextMenuEvent *event) {
    m_event.menu->move(cursor().pos());
    m_event.menu->show();
}

QList<QPointF> MWaveView::getWaveDataForChannel(WAVE_CH ch) {
    if (this->m_wave.map_series.contains(ch)) {
        return this->m_wave.map_series[ch]->points();
    }
    return QList<QPointF>();
}

void MWaveView::setYAxisAutoScale(bool enabled)
{
    m_wave.autoScaleY = enabled;
}

void MWaveView::setYAxisRange(double minY, double maxY) {
    m_wave.autoScaleY = false;
    m_wave.rangeY = maxY - minY;
    m_event.moveY = (maxY + minY) / 2.0;
    updateRange();
}

void MWaveView::setLegendVisible(bool visible) {
    if (m_wave.chart)
        m_wave.chart->legend()->setVisible(visible);
}
