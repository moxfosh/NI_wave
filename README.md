# Wave - 多功能数据采集与波形分析系统

## 项目简介

Wave是一款基于Qt开发的数据采集与波形分析系统，支持模拟输入/输出、数字IO、CAN总线和LIN总线通信。该系统提供实时波形显示、数据统计分析和CSV数据导出等功能，广泛应用于工业测试、硬件调试和信号分析领域。

**主要特性：**
- 🎯 11通道模拟输入（AI）实时采样
- 📊 2通道模拟输出（AO）波形生成
- 💾 8通道数字IO（DIO）读写控制
- 🚗 CAN总线通信（基于NI-XNET）
- 🔌 LIN总线主从通信
- 📈 实时波形显示与交互式图表
- 📉 数据统计分析（平均值、RMS、峰峰值）
- 💼 CSV数据导出
- 🎨 深色主题UI

---

## 系统架构

### 核心架构设计

项目采用模块化架构，将功能按职责拆分为独立的管理器类和工具类，主窗口仅负责UI事件处理和模块协调。代码已从原始的1100+行重构为约500行主窗口 + 多个独立管理器模块。

```
wave_2/
├── main.cpp                    # 程序入口，应用暗色主题
├── mainwindow.h/cpp/ui         # 主窗口（UI协调层）
├── managers/                   # 功能管理器层
│   ├── analoginputmanager      # 模拟输入管理
│   ├── analogoutputmanager     # 模拟输出管理
│   ├── digitalio_manager       # 数字IO管理
│   ├── canmanager              # CAN通信管理
│   └── linmanager              # LIN通信管理
├── module/                     # UI模块
│   └── wave_view/              
│       └── mwaveview           # 波形图表视图组件
├── utils/                      # 工具类
│   ├── waveformgenerator       # 波形生成工具
│   └── datastatistics          # 数据统计工具
├── GaugeWidget.cpp/h           # 仪表盘显示组件
├── include/                    # 外部库头文件
│   ├── NIDAQmx.h              # NI数据采集卡
│   ├── PCANBasic.h            # PEAK CAN接口
│   └── nixnet.h               # NI-XNET CAN/LIN
├── lib32/lib64/                # 链接库文件
├── Resource/                   # 资源文件（图标等）
├── darkstyle.qss/qrc          # 深色主题样式表
└── wave.pro                    # Qt项目配置文件
```

---

## 模块功能详解

### 1. MainWindow（主窗口）

**文件：** [mainwindow.h](mainwindow.h), [mainwindow.cpp](mainwindow.cpp), [mainwindow.ui](mainwindow.ui)

**职责：**
- UI事件处理（按钮点击、复选框切换、滑动条调节）
- 管理器生命周期管理（创建、初始化、销毁）
- 信号槽连接与数据转发
- Tab页面切换逻辑控制

**核心功能：**
- 初始化所有管理器实例
- 连接管理器信号到UI更新槽函数
- 管理通道启用状态（29个通道：11 AI + 2 AO + 8 DIO + 8 DI）
- 协调波形视图和仪表盘显示

**关键槽函数：**
- `onAIDataReady()` - 处理模拟输入数据
- `onAOWaveformGenerated()` - 处理模拟输出波形
- `onDIDataReady()` - 处理数字输入数据
- `onCANFrameReceived()` - 处理CAN帧接收
- `onLINMasterDataReceived()` - 处理LIN数据接收
- `onManagerError()` - 统一错误处理

---

### 2. AnalogInputManager（模拟输入管理器）

**文件：** [managers/analoginputmanager.h](managers/analoginputmanager.h), [managers/analoginputmanager.cpp](managers/analoginputmanager.cpp)

**职责：** 管理11个AI通道（AI0~AI10）的配置和实时采样

**硬件接口：** NI数据采集卡（通过NIDAQmx库）

**主要接口：**
```cpp
void setChannelsEnabled(const QVector<bool> &enabled);  // 设置启用的通道
void startSampling(int intervalMs);                     // 开始定时采样
void stopSampling();                                    // 停止采样
bool isSampling() const;                                // 查询采样状态
```

**信号：**
```cpp
void dataReady(int channel, const QList<QPointF> &data);  // 数据就绪
void errorOccurred(const QString &error);                  // 错误发生
```

**技术特点：**
- 动态任务重建：通道配置变化时自动重建DAQmx任务
- 批量采样：每通道采集1000个样本点（10kHz采样率）
- 定时器驱动：可配置采样间隔
- 错误处理：统一的DAQmx错误处理机制

---

### 3. AnalogOutputManager（模拟输出管理器）

**文件：** [managers/analogoutputmanager.h](managers/analogoutputmanager.h), [managers/analogoutputmanager.cpp](managers/analogoutputmanager.cpp)

**职责：** 管理2个AO通道（AO0~AO1）的波形输出

**硬件接口：** NI数据采集卡（通过NIDAQmx库）

**支持波形类型：**
- 固定电压值
- 正弦波
- 三角波
- 方波

**主要接口：**
```cpp
void setOutputChannels(const QVector<bool> &enabled);                      // 设置输出通道
void outputWaveform(WaveformGenerator::WaveType type, double amplitude);   // 输出波形
void stopOutput();                                                         // 停止输出
```

**信号：**
```cpp
void waveformGenerated(const QList<QPointF> &displayData, double voltage);  // 波形生成完成
void errorOccurred(const QString &error);                                   // 错误发生
```

**技术特点：**
- 集成WaveformGenerator工具类生成波形数据
- 支持单通道或双通道同步输出
- 1000点高分辨率波形
- 实时电压值反馈

---

### 4. DigitalIOManager（数字IO管理器）

**文件：** [managers/digitalio_manager.h](managers/digitalio_manager.h), [managers/digitalio_manager.cpp](managers/digitalio_manager.cpp)

**职责：** 管理8个DIO通道的读写操作

**硬件接口：** NI数据采集卡（通过NIDAQmx库）

**主要接口：**
```cpp
void setPort(const QString &port);                   // 设置端口（如"Dev1/port0"）
void writeDigitalOutput(const QVector<bool> &data);  // 写数字输出
void startReadingInput(int intervalMs);              // 开始定时读取输入
void stopReadingInput();                             // 停止读取
```

**信号：**
```cpp
void digitalInputRead(const QVector<bool> &data, const QVector<QPointF> &waveData);  // 数字输入读取完成
void errorOccurred(const QString &error);                                             // 错误发生
```

**技术特点：**
- 输入输出独立管理（分别创建任务）
- 定时轮询读取数字输入
- 8位并行数据转换为布尔向量
- 自动生成波形显示数据

---

### 5. CANManager（CAN通信管理器）

**文件：** [managers/canmanager.h](managers/canmanager.h), [managers/canmanager.cpp](managers/canmanager.cpp)

**职责：** 管理CAN总线接口的初始化、帧发送和接收

**硬件接口：** NI-XNET硬件（通过nixnet库）

**主要接口：**
```cpp
bool openCAN(const QString &interface, uint32_t baudRate);  // 打开CAN接口
void closeCAN();                                            // 关闭CAN接口
bool sendFrame(uint32_t id, const QByteArray &data);        // 发送CAN帧
bool isOpen() const;                                        // 查询连接状态
```

**信号：**
```cpp
void frameReceived(uint32_t id, const QByteArray &data, const QString &timestamp);  // 接收到CAN帧
void errorOccurred(const QString &error);                                            // 错误发生
```

**技术特点：**
- 双会话模式：独立的发送和接收会话
- 流模式传输：使用Frame In/Out Stream模式
- 可配置波特率（125k、250k、500k、1M等）
- 定时器轮询接收（100ms间隔）
- 时间戳记录

---

### 6. LINManager（LIN通信管理器）

**文件：** [managers/linmanager.h](managers/linmanager.h), [managers/linmanager.cpp](managers/linmanager.cpp)

**职责：** 管理LIN总线主从串口的配置和通信

**硬件接口：** 串口通信（通过QSerialPort）

**主要接口：**
```cpp
bool openPorts(const QString &master, const QString &slave, 
               qint32 baudRate, QSerialPort::DataBits dataBits, 
               QSerialPort::StopBits stopBits);     // 打开主从串口
void closePorts();                                  // 关闭串口
bool sendData(const QByteArray &data);              // 发送数据
```

**信号：**
```cpp
void masterDataReceived(const QByteArray &data);    // 主节点接收到数据
void slaveDataReceived(const QByteArray &data);     // 从节点接收到数据
void errorOccurred(const QString &error);           // 错误发生
```

**技术特点：**
- 主从双串口同时管理
- 可配置波特率、数据位、停止位
- 异步数据接收（信号驱动）
- 自动端口选择对话框

---

### 7. MWaveView（波形图表视图）

**文件：** [module/wave_view/mwaveview.h](module/wave_view/mwaveview.h), [module/wave_view/mwaveview.cpp](module/wave_view/mwaveview.cpp)

**职责：** 实时波形显示和交互式图表操作

**继承自：** QChartView（Qt Charts）

**支持通道：**
- 模拟通道：CH0~CH11（AI0~AI10 + AO）
- 数字输入通道：CH12~CH19（DI0~DI7）

**主要接口：**
```cpp
void openChannel(WAVE_CH ch);                            // 打开通道显示
void closeChannel(WAVE_CH ch);                           // 关闭通道显示
void clearChannel(WAVE_CH ch);                           // 清除通道数据
void addSeriesData(WAVE_CH ch, const QPointF &point);    // 添加单点
void addSeriesData(WAVE_CH ch, const QList<QPointF> &data);  // 添加批量数据
void setRangeX(int rangeX);                              // 设置X轴范围
void setRangeY(int rangeY);                              // 设置Y轴范围
void setYAxisRange(double minY, double maxY);            // 设置Y轴限制
void startGraph();                                       // 开始绘图
void pauseGraph();                                       // 暂停绘图
```

**交互功能：**
- 鼠标左键拖动：平移视图
- 鼠标右键拖动：缩放Y轴
- 滚轮滚动：缩放X轴
- 右键菜单：开始/暂停波形
- 图例显示/隐藏

**技术特点：**
- 多通道颜色区分
- 动态坐标轴自适应
- 平滑缩放与平移
- 高性能渲染（QLineSeries）

---

### 8. GaugeWidget（仪表盘组件）

**文件：** [GaugeWidget.cpp](GaugeWidget.cpp), [gaugewidget.h](gaugewidget.h)

**职责：** 显示模拟电压值的仪表盘界面

**主要接口：**
```cpp
void setVoltage(double voltage);  // 更新电压显示
```

**技术特点：**
- 自定义QPainter绘制
- 实时电压指示
- 视觉化数值反馈

---

### 9. WaveformGenerator（波形生成工具）

**文件：** [utils/waveformgenerator.h](utils/waveformgenerator.h), [utils/waveformgenerator.cpp](utils/waveformgenerator.cpp)

**职责：** 静态工具类，生成各种波形的数学数据

**波形类型枚举：**
```cpp
enum WaveType {
    Fixed,      // 固定值
    Sine,       // 正弦波
    Triangle,   // 三角波
    Square      // 方波
};
```

**主要方法：**
```cpp
static QList<QPointF> generate(WaveType type, double amplitude, int numPoints = 1000);
static QList<QPointF> generateFixed(double value, int numPoints);
static QList<QPointF> generateSine(double amplitude, int numPoints);
static QList<QPointF> generateTriangle(double amplitude, int numPoints);
static QList<QPointF> generateSquare(double amplitude, int numPoints);
```

**技术特点：**
- 纯数学计算，无硬件依赖
- 高精度波形生成（默认1000点）
- 归一化时间轴（0~1）

---

### 10. DataStatistics（数据统计工具）

**文件：** [utils/datastatistics.h](utils/datastatistics.h), [utils/datastatistics.cpp](utils/datastatistics.cpp)

**职责：** 静态工具类，计算统计值和导出CSV

**统计结果结构：**
```cpp
struct Statistics {
    double mean;        // 平均值
    double rms;         // 均方根值
    double peakToPeak;  // 峰峰值
    double max;         // 最大值
    double min;         // 最小值
};
```

**主要方法：**
```cpp
static Statistics calculate(const QList<QPointF> &data);
static bool exportToCSV(const QString &filePath, const QList<QList<QPointF>> &channelData);
```

**技术特点：**
- 高效统计算法
- CSV格式标准化输出
- 多通道数据批量导出

---

## 硬件依赖

### 必需硬件

1. **NI数据采集卡**
   - 用于模拟输入/输出和数字IO
   - 支持的设备：NI USB-6xxx系列、PCI-6xxx系列等
   - 需安装NI-DAQmx驱动

2. **NI-XNET硬件**（可选）
   - 用于CAN总线通信
   - 支持的设备：NI USB-8502、PCI/PCIe-8531等
   - 需安装NI-XNET驱动

3. **串口设备**（可选）
   - 用于LIN总线主从通信
   - 标准RS232/USB转串口设备

### 软件依赖

- **Qt 6.7.3** 或更高版本
- **NIDAQmx** 驱动和库（National Instruments）
- **NI-XNET** 驱动和库（National Instruments）
- **PEAK CAN Basic API**（用于PCAN设备，可选）
- **MSVC 2022** 编译器（Windows）

---

## 编译与运行

### 环境配置

1. 安装Qt 6.7.3（包含Qt Charts模块）
2. 安装NI-DAQmx驱动（从National Instruments官网下载）
3. 安装NI-XNET驱动（可选，如需CAN功能）
4. 配置环境变量：
   ```
   C:\Program Files (x86)\National Instruments\Shared\ExternalCompilerSupport\C\include
   C:\Program Files (x86)\National Instruments\Shared\ExternalCompilerSupport\C\lib64\msvc
   ```

### 编译步骤

```bash
# 进入项目目录
cd d:\QT\samples\wave_2

# 生成Makefile
qmake wave.pro

# 编译项目
make

# 或使用Qt Creator打开wave.pro直接编译
```

### 运行程序

```bash
# Windows
.\wave.exe

# 或双击生成的wave.exe文件
```

---

## 使用指南

### 1. 模拟输入（AI）采样

1. 切换到"模拟输入"标签页
2. 勾选需要启用的AI通道（AI0~AI10）
3. 调整X轴和Y轴范围滑动条
4. 点击"开始"按钮开始实时采样
5. 波形将在图表中实时显示
6. 仪表盘显示AI0通道的当前电压值
7. 点击"暂停"按钮可暂停显示
8. 点击"保存数据"导出CSV文件

### 2. 模拟输出（AO）波形生成

1. 切换到"模拟输出"标签页
2. 勾选需要输出的AO通道（AO0~AO1）
3. 选择波形类型：固定值/正弦波/三角波/方波
4. 设置输出幅值（电压）
5. 点击"输出"按钮生成波形
6. 输出波形将同时显示在图表中

### 3. 数字IO（DIO）操作

1. 切换到"数字IO"标签页
2. **数字输出：**
   - 勾选需要输出的DO通道（DO0~DO7）
   - 点击"写入DO"按钮
3. **数字输入：**
   - 系统自动读取DI0~DI7状态
   - 指示灯显示当前输入电平
   - 输入波形显示在图表中

### 4. CAN总线通信

1. 切换到"CAN"标签页
2. 选择CAN接口（如"CAN1"）
3. 选择波特率（125k/250k/500k/1M）
4. 点击"初始化CAN"按钮
5. **发送帧：**
   - 输入CAN ID（十六进制）
   - 输入数据（十六进制，如"01 02 03 04"）
   - 点击"发送"按钮
6. **接收帧：**
   - 接收到的帧显示在文本框中
   - 格式：`[时间戳] ID: 数据`

### 5. LIN总线通信

1. 切换到"LIN"标签页
2. 点击"打开LIN"按钮选择主从串口
3. 配置波特率、数据位、停止位
4. **发送数据：**
   - 输入十六进制数据
   - 点击"发送"按钮
5. **接收数据：**
   - 主节点和从节点接收的数据分别显示

---

## 设计亮点

### 1. 模块化架构

- **单一职责原则：** 每个管理器类只负责一个硬件模块
- **低耦合：** 管理器之间无直接依赖
- **易扩展：** 添加新功能只需新增管理器类

### 2. 信号槽架构

- **解耦UI与业务逻辑：** 管理器通过信号槽与MainWindow通信
- **异步处理：** 定时器驱动的非阻塞采样
- **统一错误处理：** 所有错误通过errorOccurred信号统一上报

### 3. 资源管理

- **RAII原则：** 析构函数自动清理硬件资源（关闭任务、会话等）
- **任务重建机制：** 通道配置变化时自动重建DAQmx任务
- **错误恢复：** DAQmx错误后自动清理任务状态

### 4. 性能优化

- **批量采样：** 每次采集1000个样本点减少调用开销
- **定时器驱动：** 避免轮询阻塞主线程
- **高效渲染：** Qt Charts的硬件加速绘图

### 5. 用户体验

- **深色主题：** 降低眼睛疲劳，专业工具风格
- **交互式图表：** 拖动、缩放、暂停等操作
- **实时反馈：** 仪表盘显示当前数值
- **数据导出：** CSV格式方便后续分析

---

## 后续改进计划

1. **单元测试**
   - 为管理器类添加单元测试
   - 使用Mock对象模拟硬件

2. **配置文件**
   - 使用JSON/XML配置硬件参数
   - 保存用户界面设置

3. **日志系统**
   - 记录操作历史和错误日志
   - 支持日志回放分析

4. **多线程优化**
   - 将耗时操作移到工作线程
   - 使用线程池管理并发任务

5. **插件系统**
   - 支持动态加载硬件驱动插件
   - 扩展新的总线协议支持

6. **数据库存储**
   - 将采样数据存储到SQLite数据库
   - 支持历史数据查询和比对

---

## 常见问题

### Q1: 编译时提示找不到NIDAQmx.h？
**A:** 确保已安装NI-DAQmx驱动，并在wave.pro中配置正确的INCLUDEPATH路径。

### Q2: 运行时提示"无法找到设备"？
**A:** 使用NI MAX软件确认设备已正确连接和识别，检查设备名称（如"Dev1"）是否正确。

### Q3: CAN通信无法初始化？
**A:** 确保已安装NI-XNET驱动，并在NI MAX中配置了CAN接口。

### Q4: 波形显示卡顿？
**A:** 减少采样频率或增加采样间隔，关闭不需要的通道。

### Q5: CSV导出的数据格式是什么？
**A:** 第一行为时间轴，后续行为各通道的电压值，列之间用逗号分隔。

---

**开发环境：** Qt 6.7.3 + MSVC 2022 + Windows 11  
**硬件平台：** NI数据采集卡 + NI-XNET CAN接口  
**架构风格：** 模块化 + 信号槽 + RAII资源管理
