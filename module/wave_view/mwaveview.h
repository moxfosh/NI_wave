#ifndef MWAVEVIEW_H
#define MWAVEVIEW_H

#include <QObject>
#include <QWidget>
#include <QChartView>
#include <QtCharts>

// 定义最大通道数量（包括模拟通道和数字输入通道）
#define WAVE_CHANNEL_MAX 20

// 枚举所有通道，包括模拟通道0~11和数字输入通道12~19
enum WAVE_CH {
  WAVE_CH0,
  WAVE_CH1,
  WAVE_CH2,
  WAVE_CH3,
  WAVE_CH4,
  WAVE_CH5,
  WAVE_CH6,
  WAVE_CH7,
  WAVE_CH8,
  WAVE_CH9,
  WAVE_CH10,
  WAVE_CH11,  // 模拟输出
  WAVE_CH12,  // 数字输入 DI0
  WAVE_CH13,
  WAVE_CH14,
  WAVE_CH15,
  WAVE_CH16,
  WAVE_CH17,
  WAVE_CH18,
  WAVE_CH19   // 数字输入 DI7
};

// 直线:  QLineSeries
// 平滑线  QSplineSeries
typedef QLineSeries SeriesType;

struct Wave {
  QMap<WAVE_CH, SeriesType *> map_series;  //key: ch, value SeriesType *
  QChart *chart;  // 图纸
  QValueAxis *axisX, *axisY;   // xy轴
  double rangeX, rangeY;      // 控制XY的可视范围
  double multipleX, multipleY;   // 放大倍数，对应缩小倍数为1/multiple.
  double last_point_x = 0;
  bool autoScaleY = true;       // Y轴自适应开关
};

struct Event {
  bool rightButtonPressed = false;
  bool leftButtonPressed = false;
  QPoint PressedPos;
  double moveX,moveY;  // 记录移动
  bool pauseWave;
  QMenu *menu;
  QAction* startAction;
  QAction* pauseAction;
};

class MWaveView: public QChartView {
  Q_OBJECT
public:
  MWaveView(QWidget *parent);
  ~MWaveView();
  QList<QPointF> getWaveDataForChannel(WAVE_CH ch);

  // 设置的通道数量（模拟通道+数字输入通道）
  #define SET_GLOBLE_CHANNEL  20

  // 打开某个通道显示
  void openChannel(WAVE_CH ch);
  // 关闭某个通道显示
  void closeChannel(WAVE_CH ch);

  // 清除某个通道波形数据
  void clearChannel(WAVE_CH ch);
  // 追加某个通道的坐标点
  void addSeriesData(WAVE_CH ch, const QPointF& point);
  // 传入某个通道的波形
  void addSeriesData(WAVE_CH ch, const QList<QPointF>& point_list);
  // 批量更新完所有通道后调用一次，统一刷新坐标轴
  void flushRange();

  // 设置X轴范围
  void setRangeX(int rangeX);
  // 设置Y轴范围
  void setRangeY(int rangeY);

  // 设置X轴范围放大倍数,默认1.2，不可为0，对应缩小为1/multiple
  void setZoomX(double multiple);
  // 设置Y轴范围放大倍数，默认1.2，不可为0，对应缩小为1/multiple
  void setZoomY(double multiple);

  // X轴范围按比例缩小
  void ZoomOutX(void);
  // X轴范围按比例放大
  void ZoomX(void);

  // Y轴范围按比例缩小
  void ZoomOutY(void);
  // Y轴范围按比例放大
  void ZoomY(void);

  // 波形开始
  void startGraph(void);
  // 波形暂停
  void pauseGraph(void);
  //控制图例显示/隐藏
  void setLegendVisible(bool visible);

public slots:
  // 波形开始槽函数
  void slots_startGraph(void);
  // 波形暂停槽函数
  void slots_pauseGraph(void);
  // 手动设置Y轴范围（关闭自适应）
  void setYAxisRange(double minY, double maxY);
  // 开启Y轴自适应（默认开启）
  void setYAxisAutoScale(bool enabled);

protected:
  virtual void mouseMoveEvent(QMouseEvent *pEvent) override;
  virtual void mousePressEvent(QMouseEvent *pEvent) override;
  virtual void mouseReleaseEvent(QMouseEvent *pEvent) override;
  virtual void wheelEvent(QWheelEvent *pEvent) override;
  virtual void contextMenuEvent(QContextMenuEvent *event) override;

private:
  void addChannel(WAVE_CH ch);
  void updateRange(void);

  QBoxLayout *wave_layout;
  Wave m_wave;
  Event m_event;
};

#endif // MWAVEVIEW_H
