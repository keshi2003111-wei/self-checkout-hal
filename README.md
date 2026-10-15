# 基于 STM32 与 YOLO 的自助结算终端

这是一个面向散装果蔬称重结算场景的嵌入式原型。STM32F407 负责称重、按键交互、OLED 显示、价格计算、蜂鸣器/LED 提示及热敏小票打印，并通过串口与 K230 等 AI 端通信，获取 YOLO 商品识别结果。

## 功能概览

- HX711 + 称重传感器采集商品重量，并支持去皮与标定
- 128×64 SSD1306 OLED 显示操作状态、重量及结算信息
- 支持苹果、香蕉、葡萄、柠檬、番茄、辣椒 6 类商品识别
- STM32 通过 USART2 向 AI 端发送 `DETECT`，接收 `CLASS:<id>,CONF:<0-100>`
- 按键驱动 `IDLE → WEIGHING → CONFIRM → SETTLING` 结算流程
- 热敏打印机输出商品、重量、单价、金额和合计
- 蜂鸣器与 LED 提供操作反馈
- 附带 STM32CubeMX 配置、Keil MDK 工程和 YOLO 数据集配置

## 硬件与开发环境

| 项目 | 说明 |
| --- | --- |
| MCU | STM32F407VE |
| 固件库 | STM32 HAL |
| IDE/工具链 | Keil MDK-ARM；可使用 STM32CubeMX 查看或重新生成外设配置 |
| AI 端 | K230 或其他能实现相同串口协议的推理设备 |
| 称重 | HX711 + 负载传感器 |
| 显示 | SSD1306 128×64 I²C OLED |
| 输出 | UART 热敏打印机、LED、蜂鸣器 |

当前工程主频配置为 168 MHz。具体引脚和外设参数以 `self-checkout-hal.ioc`、`Core/Inc/` 及 `Core/driver/` 为准。

## 目录结构

```text
.
├─ Core/
│  ├─ Inc/                  # CubeMX 生成的外设头文件
│  ├─ Src/                  # 初始化与主程序
│  └─ driver/               # 称重、显示、按键、价格、打印及通信模块
├─ Drivers/                 # CMSIS 与 STM32 HAL 驱动
├─ MDK-ARM/                 # Keil 工程
├─ yolo/
│  └─ prepared6/data.yaml   # 六分类数据集配置
├─ self-checkout-hal.ioc    # STM32CubeMX 工程
└─ requirements.txt         # 可选 YOLO 训练环境
```

> 原始图片、标签、训练输出和大体积压缩包默认不提交到 Git。请在本地按 `data.yaml` 的目录结构准备数据集。

## 快速开始

### 1. 编译和烧录固件

1. 安装 Keil MDK-ARM，并安装 STM32F4 Device Family Pack。
2. 打开 `MDK-ARM/self-checkout-hal.uvprojx`。
3. 选择 `self-checkout-hal` Target，编译工程。
4. 连接调试器，将程序下载到 STM32F407VE。
5. 上电时保持秤台空载，程序启动后会自动执行零点标定。

也可以使用 STM32CubeMX 打开 `self-checkout-hal.ioc` 检查时钟、GPIO、I²C、定时器和 UART 配置。若重新生成代码，请注意保留 `USER CODE` 区域及 `Core/driver/` 中的自定义驱动。

### 2. 配置 AI 串口协议

STM32 与 AI 端通过 USART2 通信，默认波特率为 115200：

```text
STM32 -> AI: DETECT\n
AI -> STM32: CLASS:3,CONF:95\n
```

类别 ID 与 `yolo/prepared6/data.yaml` 保持一致：

| ID | 类别 |
| ---: | --- |
| 0 | apple |
| 1 | banana |
| 2 | grapes |
| 3 | lemon |
| 4 | tomato |
| 5 | chili |

### 3. 训练 YOLO 模型（可选）

项目未包含 Python 推理脚本；`requirements.txt` 仅用于复现数据集训练环境。建议使用 Python 3.10 或 3.11：

```bash
python -m venv .venv
# Windows
.venv\Scripts\activate
python -m pip install -U pip
pip install -r requirements.txt
yolo detect train data=yolo/prepared6/data.yaml model=yolov8n.pt epochs=100 imgsz=640 batch=16
```

训练完成后，需要根据 AI 端运行环境导出/部署模型，并实现上面的串口协议。

## 使用流程

1. 空载上电，等待零点标定完成。
2. 放置商品并按提示进入称重。
3. 系统稳定读取重量后，确认本次商品。
4. AI 端识别商品类别并返回类别与置信度。
5. STM32 计算价格、显示结果，并在结算时打印小票。

## 标定与参数调整

- 称重比例系数：`Core/driver/HX711.h` 中的 `HX711_SCALE`
- 商品价格表：`Core/driver/PRICE.c`
- AI 通信协议：`Core/driver/AI_COMM.h` 与 `AI_COMM.c`
- 业务状态机：`Core/driver/WORKFLOW.c`、`KEY.c`

实际接线、传感器量程和打印机串口参数可能因硬件版本而异，烧录前请对照原理图及 CubeMX 配置。

## 注意事项

- 本仓库主要是嵌入式固件；STM32 HAL/CMSIS 源码已随工程提供，不通过 `pip` 安装。
- `requirements.txt` 只对应可选的 PC 端 YOLO 训练工具。
- 数据集图片和权重通常较大。如需版本管理，请使用 Git LFS 或外部数据集存储，并在 README 中补充下载地址。
- 仓库未声明开源许可证；在添加 LICENSE 前，请勿默认其可用于商业分发。

