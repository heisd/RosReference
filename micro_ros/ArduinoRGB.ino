#include <micro_ros_arduino.h>
#include <Adafruit_NeoPixel.h>
#include <WiFi.h>

#include <rcl/rcl.h>
#include <rcl/error_handling.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>

#include <std_msgs/msg/color_rgba.h>

// WiFi 配置
#define WIFI_SSID "荣耀 Magic6"
#define WIFI_PASS "lqy060510"

// micro-ROS Agent 配置
#define AGENT_IP "192.168.200.202"
#define AGENT_PORT 8888

// LED 配置
#define LED_PIN 48
#define NUM_LEDS 1

// micro-ROS 对象
rcl_subscription_t subscriber;
std_msgs__msg__ColorRGBA msg;
rclc_executor_t executor;
rclc_support_t support;
rcl_allocator_t allocator;
rcl_node_t node;

// NeoPixel 对象
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// 状态枚举
enum State {
  CONNECTING_WIFI,      // 连接WiFi中
  WAITING_AGENT,        // 等待Agent
  AGENT_AVAILABLE,      // Agent可用
  AGENT_CONNECTED,      // 已连接
  AGENT_DISCONNECTED    // 连接断开
} state;

// 显示不同状态的LED颜色
void showState() {
  switch(state) {
    case CONNECTING_WIFI:
      // 紫色闪烁：连接WiFi中
      strip.setPixelColor(0, strip.Color(128, 0, 128));
      strip.show();
      delay(200);
      strip.setPixelColor(0, strip.Color(0, 0, 0));
      strip.show();
      delay(200);
      break;
    case WAITING_AGENT:
      strip.setPixelColor(0, strip.Color(255, 255, 0)); // 黄色：等待Agent
      strip.show();
      break;
    case AGENT_AVAILABLE:
      strip.setPixelColor(0, strip.Color(0, 0, 255)); // 蓝色：发现Agent
      strip.show();
      break;
    case AGENT_CONNECTED:
      strip.setPixelColor(0, strip.Color(0, 255, 0)); // 绿色：已连接
      strip.show();
      break;
    case AGENT_DISCONNECTED:
      strip.setPixelColor(0, strip.Color(255, 0, 0)); // 红色：断开连接
      strip.show();
      break;
  }
}

// 订阅回调函数
void subscription_callback(const void * msgin) {  
  const std_msgs__msg__ColorRGBA * msg = (const std_msgs__msg__ColorRGBA *)msgin;
  
  uint8_t r = (uint8_t)(msg->r * 255.0);
  uint8_t g = (uint8_t)(msg->g * 255.0);
  uint8_t b = (uint8_t)(msg->b * 255.0);
  
  strip.setPixelColor(0, strip.Color(r, g, b));
  strip.show();
  
  // 短暂延迟后恢复状态指示
  delay(500);
  showState();
}

// 连接WiFi
bool connectWiFi() {
  Serial.print("Connecting to WiFi ");
  Serial.println(WIFI_SSID);
  
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    showState(); // 紫色闪烁
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    
    // WiFi连接成功 - 白色闪烁3次
    for(int i = 0; i < 3; i++) {
      strip.setPixelColor(0, strip.Color(255, 255, 255));
      strip.show();
      delay(200);
      strip.setPixelColor(0, strip.Color(0, 0, 0));
      strip.show();
      delay(200);
    }
    return true;
  } else {
    Serial.println("\nWiFi connection failed!");
    return false;
  }
}

// 创建 micro-ROS 实体
bool createEntities() {
  allocator = rcl_get_default_allocator();
  
  // 创建初始化选项
  rcl_ret_t ret = rclc_support_init(&support, 0, NULL, &allocator);
  if (ret != RCL_RET_OK) {
    Serial.println("Failed to init support");
    return false;
  }

  // 创建节点
  ret = rclc_node_init_default(&node, "rgb_led_node", "", &support);
  if (ret != RCL_RET_OK) {
    Serial.println("Failed to init node");
    return false;
  }

  // 创建订阅者
  ret = rclc_subscription_init_default(
    &subscriber,
    &node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, ColorRGBA),
    "led_color");
  if (ret != RCL_RET_OK) {
    Serial.println("Failed to init subscription");
    return false;
  }

  // 创建执行器
  ret = rclc_executor_init(&executor, &support.context, 1, &allocator);
  if (ret != RCL_RET_OK) {
    Serial.println("Failed to init executor");
    return false;
  }
  
  ret = rclc_executor_add_subscription(&executor, &subscriber, &msg, &subscription_callback, ON_NEW_DATA);
  if (ret != RCL_RET_OK) {
    Serial.println("Failed to add subscription");
    return false;
  }

  Serial.println("micro-ROS entities created successfully");
  return true;
}

// 销毁 micro-ROS 实体
void destroyEntities() {
  rmw_context_t * rmw_context = rcl_context_get_rmw_context(&support.context);
  (void) rmw_uros_set_context_entity_destroy_session_timeout(rmw_context, 0);

  rcl_subscription_fini(&subscriber, &node);
  rclc_executor_fini(&executor);
  rcl_node_fini(&node);
  rclc_support_fini(&support);
}

void setup() {
  // 初始化串口（用于调试）
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("Starting micro-ROS WiFi RGB LED Controller");
  
  // 初始化 NeoPixel
  strip.begin();
  strip.show();
  
  // 连接WiFi
  state = CONNECTING_WIFI;
  if (!connectWiFi()) {
    // WiFi连接失败 - 红色快闪
    while(1) {
      strip.setPixelColor(0, strip.Color(255, 0, 0));
      strip.show();
      delay(100);
      strip.setPixelColor(0, strip.Color(0, 0, 0));
      strip.show();
      delay(100);
    }
  }
  
  // 设置 micro-ROS WiFi 传输
  Serial.print("Connecting to micro-ROS agent at ");
  Serial.print(AGENT_IP);
  Serial.print(":");
  Serial.println(AGENT_PORT);
  
  set_microros_wifi_transports(WIFI_SSID, WIFI_PASS, AGENT_IP, AGENT_PORT);
  
  state = WAITING_AGENT;
  showState();
}

void loop() {
  // 检查WiFi连接
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected, reconnecting...");
    state = CONNECTING_WIFI;
    connectWiFi();
    return;
  }
  
  switch(state) {
    case WAITING_AGENT:
      // 等待 Agent 连接
      Serial.println("Waiting for agent...");
      if (RMW_RET_OK == rmw_uros_ping_agent(1000, 1)) {
        Serial.println("Agent detected!");
        state = AGENT_AVAILABLE;
        showState();
      }
      delay(500);
      break;

    case AGENT_AVAILABLE:
      // 创建 micro-ROS 实体
      Serial.println("Creating entities...");
      if (createEntities()) {
        state = AGENT_CONNECTED;
        showState();
      } else {
        Serial.println("Failed to create entities, retrying...");
        state = WAITING_AGENT;
        showState();
        delay(1000);
      }
      break;

    case AGENT_CONNECTED:
      // 正常运行
      if (RMW_RET_OK == rmw_uros_ping_agent(100, 1)) {
        rclc_executor_spin_some(&executor, RCL_MS_TO_NS(100));
      } else {
        Serial.println("Agent disconnected!");
        state = AGENT_DISCONNECTED;
        showState();
      }
      break;

    case AGENT_DISCONNECTED:
      Serial.println("Destroying entities...");
      destroyEntities();
      state = WAITING_AGENT;
      showState();
      delay(1000);
      break;
  }
  
  delay(10);
}

