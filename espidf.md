# ESP-IDF 使用参考手册（含 micro-ROS 集成）

ESP-IDF（Espressif IoT Development Framework）是乐鑫官方提供的 ESP32 系列芯片完整开发框架，内置 FreeRTOS、Wi-Fi、BLE、外设驱动等功能栈。本文在 ESP-IDF 基础上集成 micro-ROS 组件，使 ESP32 能够作为 ROS2 节点参与机器人系统通信。

## 1. 克隆 ESP-IDF

```bash
mkdir ~/ESP32 && cd ~/ESP32
# 推荐使用稳定 release 分支，避免 master 分支兼容性问题
git clone -b v5.2.1 https://github.com/espressif/esp-idf.git
cd esp-idf
```

## 2. 安装工具链并配置环境

```bash
cd ~/ESP32/esp-idf
./install.sh        # 安装编译器、调试器等工具链，依赖 python3-venv

# 导出环境变量（每次新开终端都需要执行）
. ./export.sh
```

**推荐：** 将 `export.sh` 写入 shell 配置文件，免去每次手动 source：

```bash
# bash 用户
echo '. ~/ESP32/esp-idf/export.sh' >> ~/.bashrc

# zsh 用户
echo '. ~/ESP32/esp-idf/export.sh' >> ~/.zshrc
```

## 3. 创建项目并集成 micro-ROS 组件

```bash
cd ~/ESP32

# 创建新项目
idf.py create-project my_micro_ros_project
cd my_micro_ros_project

# 添加 micro-ROS ESP-IDF 组件
mkdir -p components && cd components
git clone https://github.com/micro-ROS/micro_ros_espidf_component.git
cd ..
```

**目录结构说明：**

```
~/ESP32/
├── esp-idf/                              # ESP-IDF 框架（必须与项目同级）
└── my_micro_ros_project/
    ├── CMakeLists.txt
    ├── main/
    │   ├── CMakeLists.txt
    │   └── main.c
    └── components/
        └── micro_ros_espidf_component/   # micro-ROS 组件
```

> **重要：** 项目目录必须与 `esp-idf` 目录在同一层级，否则组件内部依赖路径会出错。

## 4. 选择目标芯片与配置项目

```bash
# 选择目标芯片（以 ESP32-S3 为例）
idf.py set-target esp32s3

# 进入图形化配置菜单
idf.py menuconfig
```

### micro-ROS 关键配置项（menuconfig 中）

进入 `micro-ROS Settings` 子菜单，配置以下参数：

| 配置项 | 推荐值 | 说明 |
|--------|--------|------|
| Transport | `WiFi UDP` | 通过 UDP 与 Agent 通信（也可选串口） |
| Agent IP | `192.168.x.x` | 运行 micro-ROS Agent 的主机 IP |
| Agent Port | `8888` | Agent 监听的 UDP 端口 |
| WiFi SSID | 你的热点名称 | ESP32 连接的 Wi-Fi 热点 |
| WiFi Password | 你的热点密码 | |

> 若使用**串口传输**（无 Wi-Fi 场景），将 Transport 改为 `Serial`，并在 Agent 端使用 `serial` 模式启动。

## 5. 编写主程序

编辑 `main/main.c`，以下是通过 Wi-Fi UDP 发布 `std_msgs/Int32` 消息的完整示例：

```c
#include <string.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <uros_network_interfaces.h>
#include <rcl/rcl.h>
#include <rcl/error_handling.h>
#include <std_msgs/msg/int32.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>
#include <rmw_microros/rmw_microros.h>

#define RCCHECK(fn) {                                           \
    rcl_ret_t rc = (fn);                                        \
    if (rc != RCL_RET_OK) {                                     \
        printf("Error at %s:%d (rc=%d)\n", __FILE__, __LINE__, (int)rc); \
        vTaskDelete(NULL);                                      \
    }                                                           \
}

void micro_ros_task(void *arg)
{
    rcl_allocator_t allocator = rcl_get_default_allocator();
    rclc_support_t support;
    RCCHECK(rclc_support_init(&support, 0, NULL, &allocator));

    rcl_node_t node;
    RCCHECK(rclc_node_init_default(&node, "esp32_node", "", &support));

    rcl_publisher_t publisher;
    RCCHECK(rclc_publisher_init_default(
        &publisher, &node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
        "esp32_counter"));

    std_msgs__msg__Int32 msg;
    msg.data = 0;

    while (1) {
        RCCHECK(rcl_publish(&publisher, &msg, NULL));
        printf("Published: %d\n", (int)msg.data);
        msg.data++;
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    vTaskDelete(NULL);
}

void app_main(void)
{
    // Wi-Fi 参数从 menuconfig 读取，无需在代码中硬编码
    ESP_ERROR_CHECK(uros_network_interface_initialize());
    xTaskCreate(micro_ros_task, "micro_ros_task", 16000, NULL, 5, NULL);
}
```

修改 `main/CMakeLists.txt`，注册主程序组件：

```cmake
idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
)
```

## 6. 编译、烧录与监控

```bash
# 仅编译
idf.py build

# 烧录到设备（自动检测串口）
idf.py flash

# 编译 + 烧录 + 打开串口监控（一步完成，推荐）
idf.py flash monitor

# 仅监控串口输出（Ctrl+] 退出）
idf.py monitor
```

> 若串口权限不足，执行 `sudo usermod -aG dialout $USER` 后重新登录。

## 7. 启动 micro-ROS Agent 并验证

ESP32 烧录完成后，在 Linux/WSL 端启动 Agent：

```bash
cd ~/microros_ws
source install/setup.zsh
ros2 run micro_ros_agent micro_ros_agent udp4 -p 8888
```

Agent 检测到 ESP32 连接后，验证节点与话题：

```bash
# 列出当前在线的 ROS2 节点
ros2 node list

# 列出当前所有话题
ros2 topic list

# 订阅 ESP32 发布的计数数据
ros2 topic echo /esp32_counter
```

正常情况下可以看到每秒递增的整数输出。

## 8. 常见问题

| 问题现象 | 可能原因 | 解决方案 |
|---------|---------|---------|
| `install.sh` 报 Python 错误 | 缺少 `python3-venv` | `sudo apt install python3-venv` |
| `micro_ros component not found` | 组件未克隆或路径层级错误 | 确认 `components/micro_ros_espidf_component` 存在，且项目与 `esp-idf` 同级 |
| 串口烧录失败（Permission denied） | 当前用户无串口权限 | `sudo usermod -aG dialout $USER` 后重新登录 |
| ESP32 不断重启，无法连接 Agent | Wi-Fi SSID/密码或 Agent IP 配置错误 | 在 `menuconfig` 中重新检查配置，或使用串口监控查看具体报错 |
| `idf.py monitor` 出现乱码 | 波特率不匹配 | 默认 115200，检查 `menuconfig → Component config → ESP System Settings → UART console baud rate` |
| 编译速度极慢 | 内存或 CPU 不足（WSL 环境常见） | 参考 [wsl.md](./wsl.md) 扩展 WSL2 虚拟内存 |
