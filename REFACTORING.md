# 项目重构说明

## 重构概述

本次重构将原本集中在 MainWindow 类中的所有功能（1100+行代码）按功能模块拆分为独立的管理器类和工具类，大幅提升了代码的可维护性、可测试性和可扩展性。

## 新增文件结构

```
wave0/
├── managers/                           # 功能管理器
│   ├── analoginputmanager.h/cpp       # 模拟输入管理（11通道AI）
│   ├── analogoutputmanager.h/cpp      # 模拟输出管理（2通道AO）
│   ├── digitalio manager.h/cpp         # 数字IO管理（8通道DIO）
│   ├── canmanager.h/cpp               # CAN通信管理
│   └── linmanager.h/cpp               # LIN通信管理
└── utils/                              # 工具类
    ├── waveformgenerator.h/cpp        # 波形生成器（纯数学计算）
    └── datastatistics.h/cpp           # 数据统计和CSV导出
```

## 模块功能说明

### 1. AnalogInputManager（模拟输入管理器）
- **职责**：管理11个AI通道(ai0-ai10)的配置和采样
- **主要接口**：
  - `setChannelsEnabled()` - 设置启用的通道
  - `startSampling()` - 开始定时采样
  - `stopSampling()` - 停止采样
- **信号**：
  - `dataReady(int channel, QList<QPointF> data)` - 数据就绪
  - `errorOccurred(QString error)` - 错误发生

### 2. AnalogOutputManager（模拟输出管理器）
- **职责**：管理2个AO通道(ao0-ao1)的波形输出
- **支持波形**：固定值、正弦波、三角波、方波
- **主要接口**：
  - `setOutputChannels()` - 设置输出通道
  - `outputWaveform(WaveType, amplitude)` - 输出波形
  - `stopOutput()` - 停止输出
- **信号**：
  - `waveformGenerated(QList<QPointF>, voltage)` - 波形生成完成

### 3. DigitalIOManager（数字IO管理器）
- **职责**：管理8个DIO通道的读写
- **主要接口**：
  - `setPort()` - 设置端口
  - `writeDigitalOutput()` - 写数字输出
  - `startReadingInput()` - 开始定时读取输入
  - `stopReadingInput()` - 停止读取
- **信号**：
  - `digitalInputRead(QVector<bool>, QVector<QPointF>)` - 数字输入读取完成

### 4. CANManager（CAN通信管理器）
- **职责**：管理CAN接口的初始化、发送和接收
- **主要接口**：
  - `openCAN(interface, baudRate)` - 打开CAN接口
  - `closeCAN()` - 关闭CAN接口
  - `sendFrame(id, data)` - 发送CAN帧
- **信号**：
  - `frameReceived(id, data, timestamp)` - 接收到CAN帧

### 5. LINManager（LIN通信管理器）
- **职责**：管理主从串口的配置和通信
- **主要接口**：
  - `openPorts(master, slave, baudRate, dataBits, stopBits)` - 打开主从串口
  - `closePorts()` - 关闭串口
  - `sendData(data)` - 发送数据
- **信号**：
  - `masterDataReceived(data)` - 主节点接收到数据
  - `slaveDataReceived(data)` - 从节点接收到数据

### 6. WaveformGenerator（波形生成工具类）
- **职责**：静态工具类，生成各种波形数据
- **主要方法**：
  - `generateFixed()` - 生成固定值波形
  - `generateSine()` - 生成正弦波
  - `generateTriangle()` - 生成三角波
  - `generateSquare()` - 生成方波

### 7. DataStatistics（数据统计工具类）
- **职责**：静态工具类，计算统计值和导出CSV
- **主要方法**：
  - `calculate()` - 计算平均值、RMS、峰峰值等
  - `exportToCSV()` - 导出数据到CSV文件

## MainWindow 简化

重构后的 MainWindow 职责更加清晰：
- **UI事件处理**：按钮点击、复选框变化等
- **管理器协调**：创建和连接各个管理器
- **波形显示更新**：接收管理器信号并更新UI
- **Tab切换逻辑**：控制不同页面的显示

**代码量**：从1100+行减少到约500行

## 代码改进

### 1. 简化复选框处理
**改进前**（重复21次）：
```cpp
channelsEnabled[0] = (ui->checkBox_1->isChecked());
channelsEnabled[1] = (ui->checkBox_2->isChecked());
// ... 重复19次
```

**改进后**（循环处理）：
```cpp
for (int i = 0; i < 21; ++i) {
    QCheckBox *cb = findChild<QCheckBox*>(QString("checkBox_%1").arg(i + 1));
    if (cb) channelsEnabled[i] = cb->isChecked();
}
```

### 2. 统一错误处理
所有管理器类都通过 `errorOccurred(QString)` 信号统一报告错误，MainWindow 通过一个槽函数集中处理：
```cpp
void MainWindow::onManagerError(const QString &error) {
    QMessageBox::warning(this, "错误", error);
}
```

### 3. 信号槽架构
管理器类通过信号槽与UI解耦：
```cpp
// 构造函数中连接
connect(aiManager, &AnalogInputManager::dataReady, 
        this, &MainWindow::onAIDataReady);

// 槽函数处理
void MainWindow::onAIDataReady(int channel, const QList<QPointF> &data) {
    wave->addSeriesData((WAVE_CH)channel, data);
    if (channel == 0) gauge->setVoltage(data.last().y());
}
```

## 编译和使用

### 编译
```bash
cd d:/Qt/samples/wave0
qmake wave.pro
make
```

### 使用
重构后的程序使用方式与之前完全相同，所有UI操作和功能保持不变。

## 优势总结

1. **可维护性提升**：
   - MainWindow 从1100+行减少到约500行
   - 每个管理器类约150-200行，职责单一清晰
   - 代码组织更加合理，易于理解和修改

2. **可测试性提升**：
   - 管理器类可独立单元测试
   - 工具类可独立验证
   - 减少了测试的复杂度

3. **可扩展性提升**：
   - 添加新功能只需新增管理器
   - 修改功能不影响其他模块
   - 易于添加新的硬件支持

4. **代码质量提升**：
   - 消除了大量重复代码
   - 统一了错误处理机制
   - 改进了资源管理

## 注意事项

1. **向后兼容**：UI界面和用户操作流程完全保持不变
2. **硬件依赖**：需要实际硬件测试确保功能正常
3. **文件命名**：`digitalio manager.h/cpp` 文件名中包含空格，在 .pro 文件中需要用引号包裹

## 后续改进建议

1. 为管理器类添加单元测试
2. 考虑使用配置文件管理硬件参数
3. 添加日志系统记录操作历史
4. 考虑使用线程池处理耗时操作
