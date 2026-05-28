# SimpleSpectro

单通道太赫兹频谱采集与显示程序，从 `ThzOpticalSpectro` 项目中提取并简化，支持 Windows / Linux 跨平台运行。

**频率范围**: CH1 200–910 GHz  
**通信方式**: 直连 UDP（替代 libpcap）  
**信号处理**: DBI 数字后端宽频重建  
**显示**: QPainter 笛卡尔坐标系波形绘制

---

## 目录结构

```
SimpleSpectro/
├── CMakeLists.txt          # CMake 构建 (Qt5/Qt6 自动检测)
├── main.cpp                # 程序入口
├── mainwindow.h/cpp        # 主窗口 UI + 信号槽
├── udpreceiver.h/cpp       # QUdpSocket UDP 接收器
├── analysis.h/cpp          # 单通道 ADC 数据分析引擎
├── compat.h                # 跨平台兼容宏 (malloc_usable_size)
├── Axis/                   # 坐标轴 + 波形绘制组件
│   ├── XYAxis.h/cpp        #   刻度计算与渲染
│   └── XYView.h/cpp        #   QPainter 曲线绘制 + 交互
├── DBI/                    # DBI 数字后端处理管线 (纯 C++)
│   ├── dbi.h/cpp           #   主处理流程
│   ├── carrier.h/cpp       #   载波相位恢复与去除
│   ├── sync.h/cpp          #   通道间同步对齐
│   ├── ffe.h/cpp           #   MISO 前馈均衡器
│   ├── conv_same.h/cpp     #   等长卷积
│   ├── xcorr.h/cpp         #   互相关
│   ├── circshift.h/cpp     #   循环移位
│   ├── resample.h/cpp      #   重采样
│   └── operate_file.h/cpp  #   滤波器系数文件读写
├── Util/                   # 工具函数
│   ├── util.h/cpp          #   峰值查找、插值 (Spline 替代 Boost)
│   └── spline.h/cpp        #   三次样条插值
└── Config/                 # 运行时配置文件
    ├── Calibration/        #   每次开机需校准 (w_miso, sync_head)
    ├── Filters/            #   固定 DBI 滤波器 (ft_before/after_mixer)
    └── nodata.csv          #   无信号基线数据
```

## 数据流

```
[ADC 硬件] ──UDP──> [QUdpSocket]
                        │ 1005 字节原始包 (4ch × 192B int8)
                        ▼
               [UdpReceiver]
                        │ QVector<QByteArray>
                        ▼
               [Analysis::handleChannelData()]
                        │ 解析为 ch1/ch2/ch3/ch4 (QByteArray)
                        ▼
               [Analysis::funcPcap()]
                        │ DBI_process() → double* output_data
                        ▼
               [Analysis::funcADC()]
                        │ 每 2400 点分段 → QVector<QVector<float>> OSCData
                        │ 首帧 → QVector<pair<float,float>> waveData
                        ▼
               ┌────────┼────────┐
               ▼        ▼        ▼
          [XYView]  [CSV导出]  [帧导航]
          QPainter   DBI原始    切换frameId
          曲线绘制   数据导出    重新构建waveData
```

## 依赖

- **Qt 5.12+** 或 **Qt 6.x** (Widgets + Network 模块)
- **CMake 3.16+**
- C++14 编译器 (GCC 7+, MSVC 2017+, MinGW 8.1+)

## 编译

### Linux

```bash
cd SimpleSpectro
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

### Windows (MinGW)

```cmd
cd /d F:\4_CodingFiles\Qt\SimpleSpectro
mkdir build
cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
mingw32-make -j8
```

### Windows 打包发布

`windeployqt` 可能无法自动找到平台插件，需要手动复制所有依赖：

```powershell
# 在 build 目录下执行

# 1. 平台插件 (必须)
mkdir platforms
copy D:\Qt5.15\5.15.2\mingw81_64\plugins\platforms\qwindows.dll platforms\

# 2. Qt DLL
copy D:\Qt5.15\5.15.2\mingw81_64\bin\Qt5Core.dll .
copy D:\Qt5.15\5.15.2\mingw81_64\bin\Qt5Gui.dll .
copy D:\Qt5.15\5.15.2\mingw81_64\bin\Qt5Network.dll .
copy D:\Qt5.15\5.15.2\mingw81_64\bin\Qt5Widgets.dll .

# 3. MinGW 运行时
copy D:\Qt5.15\5.15.2\mingw81_64\bin\libgcc_s_seh-1.dll .
copy D:\Qt5.15\5.15.2\mingw81_64\bin\libstdc++-6.dll .
copy D:\Qt5.15\5.15.2\mingw81_64\bin\libwinpthread-1.dll .

# 4. 配置文件
copy ..\Config .\Config -Recurse
```

> **注意**: Qt 安装路径 `D:\Qt5.15\5.15.2\mingw81_64` 请根据实际环境修改。Qt6 用户路径类似 `D:\Qt\6.x\mingw_64`。

## 运行

```bash
./SimpleSpectro
```

程序启动时自动加载 `Config/nodata.csv` 作为无信号基线并开始自动轮播帧。

## 操作流程

```
[1. Bind NET] → [2. Trigger ADC] → 数据接收 → 自动轮播帧
                                                ↓
                                          点 [Stop] 暂停
                                                ↓
                                  < Prev / Next > 逐帧手动浏览
                                                ↓
                                [3. Save CSV] 导出 DBI 原始数据
                                                ↓
                              点 [Clear] → 恢复基线波形
                                                ↓
                              点 [1. Unbind NET] 断开连接
```

### 按钮说明

| 按钮 | 颜色 | 功能 |
|------|------|------|
| **1. Bind NET** / **1. Unbind NET** | 蓝 / 红 | 绑定 / 解绑 UDP 端口 (默认 10.10.229.1:5506) |
| **2. Trigger ADC** | 橙 | 发送 "mv424" 触发命令给 ADC 硬件 (默认 10.10.229.11:5506) |
| **Stop** / **Run** | 红 / 绿 | 暂停 / 恢复帧自动播放 (12 fps) |
| **3. Save CSV** | 绿 | 导出 DBI 处理后帧分割前的完整数据 (每行一个 float) |
| **< Prev** / **Next >** | 默认 | 手动逐帧浏览 (仅 Stop 后可用) |
| **Clear** | 紫 | 清除采集数据并恢复无信号基线 |

## 配置文件

### Calibration/ (每次开机需更新)

| 文件 | 说明 |
|------|------|
| `w_miso_dev1_1.txt` | CH1 MISO 均衡器系数 #1 |
| `w_miso_dev1_2.txt` | CH1 MISO 均衡器系数 #2 |
| `w_miso_dev1_3.txt` | CH1 MISO 均衡器系数 #3 |
| `sync_head_dev1.txt` | CH1 通道间同步延迟 |

### Filters/ (部署后不变)

| 文件 | 说明 |
|------|------|
| `ft_before_mixer.txt` | 混频前滤波器系数 (101 阶) |
| `ft_after_mixer_1.txt` | 混频后滤波器系数 #1 (101 阶) |
| `ft_after_mixer_2.txt` | 混频后滤波器系数 #2 (101 阶) |

### nodata.csv

无信号基线频谱数据 (998 帧 × 2727 点/帧，逗号分隔)。可用实际无信号测量替换。

## TestSender — UDP 模拟发送器

位于 `../TestSender/`，用于在没有 ADC 硬件的情况下测试主程序。

### 功能

- 读取 DBI 输出 CSV 文件
- **逆 DBI 处理**：FIR 滤波器组将宽频信号分解为 3 个 ADC 通道 (0–20 / 20–40 / 40–60 GHz)
- 3 倍抽取 + int8 量化
- 封装为 1005 字节 UDP 包并发送
- 自动发送 EOF 结束标记触发主程序分析

### 编译与运行

```bash
cd ../TestSender
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
./TestSender
```

### 操作流程

```
1. [...] 加载 DBI 输出 CSV 文件 → 自动计算包数
2. 确认目标 IP:Port (默认 10.10.229.1:5506)
3. 点击 Send → 逆 DBI → 逐包发送 → EOF
4. 主程序 SimpleSpectro 收到数据 → DBI 处理 → 显示波形
```
