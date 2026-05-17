# Soil Monitoring System

STM32F103C8 土壤监测系统 — 基于 HAL 库开发

## 功能

- 土壤湿度采集（ADC）
- OLED 显示屏驱动（I2C / SSD1306）
- WiFi 连接 + MQTT 上报（OneNet 云平台）
- 继电器控制
- 按键交互
- 调度器管理多任务

## 项目结构

```
APP/          应用层 — OLED、按键、调度器、继电器、土壤监测、串口
Core/Inc/     HAL 头文件 — ADC、GPIO、I2C、TIM、UART、DMA
Core/Src/     HAL 源码及 main.c
Net/          网络层 — WiFi连接、MQTT协议、OneNet平台、cJSON、Base64、HMAC-SHA1
soil.ioc      STM32CubeMX 工程配置
startup.s     启动文件（STM32F103xB）
```

## 硬件平台

- **MCU**: STM32F103C8T6
- **开发工具**: STM32CubeMX + Keil MDK
- **云平台**: OneNet（MQTT 协议）

## 使用说明

1. 用 STM32CubeMX 打开 `soil.ioc` 生成 HAL 代码
2. 用 Keil MDK 打开工程编译下载
3. 根据实际硬件修改 `Net/Common.h` 中的 WiFi / OneNet 配置
