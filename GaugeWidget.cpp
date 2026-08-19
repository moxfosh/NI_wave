#include "gaugewidget.h"
#include <QPainter>
#include <QtMath>

GaugeWidget::GaugeWidget(QWidget *parent)
    : QWidget(parent), m_voltage(0.0)
{
    setMinimumSize(200, 200);
}

void GaugeWidget::setVoltage(double voltage)
{
    m_voltage = qBound(-10.0, voltage, 10.0);
    update(); // 触发绘图
}

void GaugeWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    int w = width();
    int h = height();
    int side = qMin(w, h);

    p.translate(w / 2, h / 2);              // 移动坐标系中心
    p.scale(side / 200.0, side / 200.0);    // 缩放到 [-100,100]

    // 彩色刻度圆环
    QConicalGradient grad(0, 0, 225);  // 起点角度
    grad.setColorAt(0.00, QColor(0, 255, 255));   // 蓝
    grad.setColorAt(0.33, QColor(0, 255, 0));     // 绿
    grad.setColorAt(0.66, QColor(255, 255, 0));   // 黄
    grad.setColorAt(1.00, QColor(255, 0, 0));     // 红

    p.setPen(QPen(QBrush(grad), 12));
    QRectF arcRect(-85, -85, 170, 170);
    p.drawArc(arcRect, 45 * 16, 270 * 16);  // 从45°画到315°

    // 刻度线 + 数字
    p.setPen(QColor(230, 230, 230));
    QFont font = p.font();
    font.setPointSize(9);
    p.setFont(font);

    for (int i = -10; i <= 10; ++i) {
        double angle = 225 - (i + 10) * (270.0 / 20);
        double rad = qDegreesToRadians(angle);

        QPointF p1(qCos(rad) * 78, -qSin(rad) * 78);
        QPointF p2(qCos(rad) * 88, -qSin(rad) * 88);
        p.drawLine(p1, p2);

        // 数字
        QString num = QString::number(i);
        QFontMetrics fm(p.font());
        int textWidth = fm.horizontalAdvance(num);
        int textHeight = fm.height();

        QPointF center(qCos(rad) * 60, -qSin(rad) * 60);
        QPointF topLeft(center.x() - textWidth / 2.0, center.y() - 6);
        p.drawText(QRectF(topLeft, QSizeF(textWidth, textHeight)), Qt::AlignCenter, num);
    }

    // 指针（红色）
    double angle = 225 - (m_voltage + 10) * (270.0 / 20);
    double rad = qDegreesToRadians(angle);
    QPointF end(qCos(rad) * 65, -qSin(rad) * 65);

    p.setPen(QPen(QColor(255, 80, 80), 4, Qt::SolidLine, Qt::RoundCap));
    p.drawLine(QPointF(0, 0), end);

    // 中心点
    p.setPen(Qt::NoPen);
    p.setBrush(Qt::gray);
    p.drawEllipse(QPointF(0, 0), 6, 6);

    // 中心电压值显示
    p.setPen(Qt::white);
    QFont valFont("Segoe UI", 14, QFont::Bold);
    p.setFont(valFont);
    QString valStr = QString::number(m_voltage, 'f', 2);
    p.drawText(QRect(-40, 50, 80, 30), Qt::AlignCenter, valStr + " V");
}
