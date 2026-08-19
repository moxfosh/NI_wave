#ifndef GAUGEWIDGET_H
#define GAUGEWIDGET_H

#include <QWidget>

class GaugeWidget : public QWidget
{
    Q_OBJECT

public:
    explicit GaugeWidget(QWidget *parent = nullptr);
    void setVoltage(double voltage);  // 更新电压

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    double m_voltage;  // 当前电压值
};

#endif // GAUGEWIDGET_H
