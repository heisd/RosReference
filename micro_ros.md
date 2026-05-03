# micro-ROS 操作与使用指南

> 本文记录了在 ESP32-S3 上使用 micro-ROS（通过 Arduino IDE）控制 RGB LED 与舵机的完整流程。  
> 参考链接：<https://www.hackster.io/514301/micro-ros-on-esp32-using-arduino-ide-1360ca>

## 1. 在 Linux（WSL）端安装 micro-ROS Agent

```bash
cd ~
mkdir microros_ws && cd microros_ws
git clone -b humble https://github.com/micro-ROS/micro_ros_setup.git src/micro_ros_setup

# 安装 rosdep
sudo apt install python3-rosdep2
sudo apt update && rosdep update
rosdep install --from-paths src --ignore-src -y

# 安装 pip3
sudo apt-get install python3-pip

# 编译 micro_ros_setup 包
colcon build --symlink-install
source install/setup.zsh

# 创建 Agent 工作空间并编译
ros2 run micro_ros_setup create_agent_ws.sh
colcon build --symlink-install
```

## 2. 在 Windows 端安装 Arduino IDE 并配置 ESP32

### 2.1 安装 micro_ros_arduino 库

在 Arduino IDE 的「库管理器」中搜索 `micro_ros_arduino`，点击安装。

![micro_ros_arduino](./picture/micro_ros/ArduinoLib.png)

同理，搜索并安装 `Adafruit_NeoPixel` 库（用于驱动板载 RGB LED）。

### 2.2 烧录 RGB LED 控制程序到 ESP32-S3

完整代码见：[ArduinoRGB.ino](./micro_ros/ArduinoRGB.ino)

**烧录前必须修改的配置：**

| 宏定义 | 含义 | 示例值 |
|--------|------|--------|
| `WIFI_SSID` | 手机热点名称 | `"MyPhone"` |
| `WIFI_PASS` | 手机热点密码 | `"12345678"` |
| `AGENT_IP` | micro-ROS Agent 的 IP 地址 | `"192.168.43.1"` |
| `AGENT_PORT` | Agent 监听端口 | `8888` |

**获取 Agent IP 地址的方法：** 在 Windows 端执行以下命令，找到手机热点网络适配器对应的「默认网关」即为手机热点地址：

```powershell
ipconfig
```

**注意：** 开发板类型需选择 `ESP32S3 Dev Module`，而非普通 `ESP32`。

### 2.3 LED 状态指示说明

程序烧录后，LED 会以颜色指示当前连接状态：

| LED 颜色 | 含义 |
|---------|------|
| 紫色闪烁 | 正在连接 Wi-Fi |
| 红色快闪 | Wi-Fi 连接失败（请检查 SSID/密码） |
| 黄色常亮 | Wi-Fi 已连接，等待 Agent |
| 蓝色常亮 | 检测到 Agent，正在初始化 |
| 绿色常亮 | **已成功连接 Agent，可以发送指令** |
| 红色常亮 | 与 Agent 断开连接 |

## 3. 在 WSL 端运行 micro-ROS Agent

```bash
cd ~/microros_ws
source install/setup.zsh
ros2 run micro_ros_agent micro_ros_agent udp4 -p 8888
```

Agent 启动后，等待 ESP32 上的 LED 变为绿色，即表示连接成功。

## 4. 通过 ROS2 Topic 控制 RGB LED

连接成功后，新开一个终端，使用 `ros2 topic pub` 向 `/led_color` 话题发布颜色指令：

```bash
# 红色
ros2 topic pub --once /led_color std_msgs/msg/ColorRGBA "{r: 1.0, g: 0.0, b: 0.0, a: 1.0}"

# 绿色
ros2 topic pub --once /led_color std_msgs/msg/ColorRGBA "{r: 0.0, g: 1.0, b: 0.0, a: 1.0}"

# 蓝色
ros2 topic pub --once /led_color std_msgs/msg/ColorRGBA "{r: 0.0, g: 0.0, b: 1.0, a: 1.0}"

# 紫色
ros2 topic pub --once /led_color std_msgs/msg/ColorRGBA "{r: 0.5, g: 0.0, b: 0.5, a: 1.0}"

# 白色
ros2 topic pub --once /led_color std_msgs/msg/ColorRGBA "{r: 1.0, g: 1.0, b: 1.0, a: 1.0}"

# 橙色
ros2 topic pub --once /led_color std_msgs/msg/ColorRGBA "{r: 1.0, g: 0.5, b: 0.0, a: 1.0}"
```

> `--once` 参数表示只发布一次后退出，适合手动调试。若需持续控制，去掉该参数即可。

可通过以下命令确认话题存在：

```bash
ros2 topic list
ros2 topic echo /led_color
```

## 5. 通过 micro-ROS 控制舵机（ESP32 串口方式）

本节介绍如何通过 micro-ROS 订阅话题，在 ESP32 下位机端控制舵机转动。

### 5.1 硬件连接

| ESP32-S3 引脚 | 舵机接线颜色 | 说明 |
|--------------|------------|------|
| GPIO 13（可自定义） | 橙色 / 黄色 | 信号线（PWM） |
| 5V 外部电源 | 红色 | 电源线 |
| GND | 棕色 / 黑色 | 地线 |

> **建议：** 舵机使用独立 5V 电源供电，不要直接接 ESP32 的 5V 引脚，否则可能因电流不足导致 ESP32 复位或舵机抖动。

### 5.2 安装依赖库

在 Arduino IDE 库管理器中搜索并安装 `ESP32Servo` 库。

### 5.3 完整代码

```cpp
#include <micro_ros_arduino.h>
#include <rcl/rcl.h>
#include <rcl/error_handling.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>
#include <std_msgs/msg/int32.h>
#include <ESP32Servo.h>

// Wi-Fi 与 Agent 配置（根据实际情况修改）
#define WIFI_SSID    "YourSSID"
#define WIFI_PASS    "YourPassword"
#define AGENT_IP     "192.168.x.x"
#define AGENT_PORT   8888

#define SERVO_PIN    13

rcl_node_t node;
rcl_subscription_t subscriber;
std_msgs__msg__Int32 msg;
rclc_executor_t executor;
rclc_support_t support;
rcl_allocator_t allocator;

Servo myServo;

#define RCCHECK(fn) { if ((fn) != RCL_RET_OK) { while(1) { delay(100); } } }

// 收到角度值后驱动舵机
void servo_callback(const void *msgin) {
    const std_msgs__msg__Int32 *m = (const std_msgs__msg__Int32 *)msgin;
    int angle = constrain(m->data, 0, 180);
    myServo.write(angle);
}

void setup() {
    myServo.attach(SERVO_PIN);
    myServo.write(90);  // 初始位置居中

    set_microros_wifi_transports(WIFI_SSID, WIFI_PASS, AGENT_IP, AGENT_PORT);
    delay(2000);

    allocator = rcl_get_default_allocator();
    RCCHECK(rclc_support_init(&support, 0, NULL, &allocator));
    RCCHECK(rclc_node_init_default(&node, "servo_node", "", &support));

    // 订阅 /servo_angle 话题，消息类型为 Int32（角度 0~180）
    RCCHECK(rclc_subscription_init_default(
        &subscriber, &node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
        "/servo_angle"));

    RCCHECK(rclc_executor_init(&executor, &support.context, 1, &allocator));
    RCCHECK(rclc_executor_add_subscription(
        &executor, &subscriber, &msg, &servo_callback, ON_NEW_DATA));
}

void loop() {
    rclc_executor_spin_some(&executor, RCL_MS_TO_NS(100));
    delay(10);
}
```

### 5.4 发送舵机角度指令

Agent 运行后，在 Linux 端发送角度：

```bash
# 转到 90 度（居中）
ros2 topic pub --once /servo_angle std_msgs/msg/Int32 "{data: 90}"

# 转到 0 度
ros2 topic pub --once /servo_angle std_msgs/msg/Int32 "{data: 0}"

# 转到 180 度
ros2 topic pub --once /servo_angle std_msgs/msg/Int32 "{data: 180}"
```

### 5.5 常见问题

| 现象 | 原因 | 解决方案 |
|------|------|---------|
| 舵机抖动或不动 | 供电不足 | 改用外部 5V 独立供电 |
| ESP32 反复复位 | 大电流舵机拉低电压 | 同上，分离供电 |
| 话题发布无响应 | Wi-Fi 未连接或 Agent 未启动 | 检查 LED 状态，确认 Agent 在运行 |
| 角度超出范围 | 发送了负数或大于 180 的值 | 代码中已用 `constrain` 限制，正常不会出现 |
