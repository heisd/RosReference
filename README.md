# RosReference

本仓库是一份面向 **ROS2 机器人开发**的个人学习笔记与参考手册，涵盖嵌入式端（ESP32 + micro-ROS）与上位机端（MoveIt2 运动规划）的完整开发流程，并附有 WSL2 环境优化等实用技巧。

---

## 目录结构

```
RosReference/
├── micro_ros.md          # micro-ROS 操作与使用指南（Arduino IDE + ESP32）
├── espidf.md             # ESP-IDF 使用参考手册（含 micro-ROS 集成）
├── moveit.md             # MoveIt2 配置与使用指南（Panda 机械臂）
├── wsl.md                # WSL2 扩展虚拟内存指南
├── micro_ros/
│   └── ArduinoRGB.ino    # ESP32 RGB LED 控制示例（micro-ROS 订阅者）
├── ESP32/
│   └── my_micro_ros_project/   # ESP-IDF + micro-ROS 项目模板
├── moveit_error/
│   └── modify.md         # MoveIt 功能包资源文件安装修复
├── robot/                # ROS2 colcon 工作空间
└── picture/              # 文档配图
```

---

## 各文档说明

### [micro_ros.md](./micro_ros.md) — micro-ROS 操作与使用

介绍如何在 **ESP32-S3** 上通过 Arduino IDE 使用 micro-ROS，实现与 ROS2 主机的 UDP 通信。

**主要内容：**
- 在 WSL/Linux 端安装并运行 micro-ROS Agent
- 在 Windows 端配置 Arduino IDE 与相关库
- 烧录 RGB LED 控制程序，通过 `ros2 topic pub` 远程控制 LED 颜色
- 通过 micro-ROS 订阅话题控制舵机转动（含完整 Arduino 代码）

**涉及技术：** ROS2 Humble · micro-ROS · ESP32-S3 · Arduino IDE · Adafruit NeoPixel

---

### [espidf.md](./espidf.md) — ESP-IDF 使用参考手册

介绍如何使用乐鑫官方 **ESP-IDF** 框架开发 ESP32，并集成 micro-ROS 组件。

**主要内容：**
- 克隆 ESP-IDF 并配置工具链环境
- 创建项目并集成 `micro_ros_espidf_component`
- menuconfig 关键参数配置（传输方式、Agent IP、Wi-Fi 等）
- 完整的 `main.c` 发布者示例
- 编译、烧录、监控一体化命令
- 常见错误排查表

**涉及技术：** ESP-IDF v5.x · FreeRTOS · micro-ROS · ESP32-S3

---

### [moveit.md](./moveit.md) — MoveIt2 配置与使用指南

记录使用 **MoveIt Setup Assistant** 对 Franka Panda 机械臂进行完整配置的流程，以及在 WSL2 环境下排查启动崩溃问题的经历。

**主要内容：**
- 安装 MoveIt Setup Assistant 并获取 URDF（UR5e / Panda 两种方案）
- WSL2 环境崩溃问题的根本原因与解决方案（`QT_QPA_PLATFORM=xcb`）
- Setup Assistant 12 步完整配置流程（碰撞矩阵 → 规划组 → 控制器 → 生成包）
- 编译运行配置包并在 RViz2 中验证
- 通过 MoveIt Python API 编程控制机械臂（进阶）

**涉及技术：** ROS2 Humble · MoveIt2 · RViz2 · Franka Panda · ros2_control

---

### [wsl.md](./wsl.md) — WSL2 扩展虚拟内存

介绍如何通过 `.wslconfig` 为 WSL2 配置更大的物理内存上限和交换空间，解决编译 ROS2 包时速度过慢或内存不足的问题。

**主要内容：**
- 配置 `memory` 与 `swap` 参数
- 重启 WSL2 使配置生效
- 验证内存配置
- 进一步优化编译速度的技巧（并行编译、关闭杀毒扫描、使用 Linux 原生文件系统）

---

### [moveit_error/modify.md](./moveit_error/modify.md) — MoveIt 资源文件修复

记录自定义功能包因 CMakeLists.txt 缺少 `install` 指令而导致 mesh 文件未安装、MoveIt 加载失败的问题及修复方法。

---

## 环境要求

| 组件 | 版本 |
|------|------|
| ROS2 | Humble Hawksbill |
| Ubuntu | 22.04 LTS（或 WSL2） |
| ESP-IDF | v5.2.x |
| Arduino IDE | 2.x |
| MoveIt2 | humble 分支 |
| Python | 3.10+ |

---

## 快速开始

### 嵌入式方向（micro-ROS + ESP32）

```bash
# 1. 安装 micro-ROS Agent
mkdir ~/microros_ws && cd ~/microros_ws
git clone -b humble https://github.com/micro-ROS/micro_ros_setup.git src/micro_ros_setup
colcon build --symlink-install && source install/setup.zsh
ros2 run micro_ros_setup create_agent_ws.sh
colcon build --symlink-install

# 2. 启动 Agent
source install/setup.zsh
ros2 run micro_ros_agent micro_ros_agent udp4 -p 8888
```

详细步骤见 [micro_ros.md](./micro_ros.md) 和 [espidf.md](./espidf.md)。

### 机械臂方向（MoveIt2）

```bash
# 1. 安装 MoveIt Setup Assistant
sudo apt install ros-humble-moveit-setup-assistant

# 2. 克隆 Panda 资源包
mkdir ~/moveit_ws && cd ~/moveit_ws
git clone -b humble https://github.com/ros-planning/moveit_resources.git src/moveit_resources
colcon build --symlink-install && source install/setup.zsh

# 3. 启动配置工具
ros2 launch moveit_setup_assistant setup_assistant.launch.py
```

详细步骤见 [moveit.md](./moveit.md)。

---

## 参考资料

- [micro-ROS on ESP32 using Arduino IDE](https://www.hackster.io/514301/micro-ros-on-esp32-using-arduino-ide-1360ca)
- [MoveIt2 官方文档](https://moveit.picknik.ai/humble/index.html)
- [ESP-IDF 编程指南](https://docs.espressif.com/projects/esp-idf/zh_CN/latest/)
- [ROS2 Humble 文档](https://docs.ros.org/en/humble/)
