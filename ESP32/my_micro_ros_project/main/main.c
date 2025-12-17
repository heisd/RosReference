#include <rcl/rcl.h>
#include <rcl/error_handling.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>
#include <std_msgs/msg/int32.h>
#include <rmw_microros/rmw_microros.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

// 如果用 WiFi 传输
#include <uros_network_interfaces.h>

static const char *TAG = "micro_ros";

void micro_ros_task(void *arg)
{
    rcl_allocator_t allocator = rcl_get_default_allocator();
    rclc_support_t support;

    // 等待 agent 连接
    rmw_uros_set_custom_transport(
        true, NULL,
        pico_serial_transport_open,   // 或自定义的 WiFi transport
        pico_serial_transport_close,
        pico_serial_transport_write,
        pico_serial_transport_read
    );

    // 初始化 support
    rclc_support_init(&support, 0, NULL, &allocator);

    // 创建 node
    rcl_node_t node;
    rclc_node_init_default(&node, "esp32_node", "", &support);

    // 创建 publisher
    rcl_publisher_t publisher;
    rclc_publisher_init_default(
        &publisher, &node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
        "esp32_publisher"
    );

    // 发布消息循环
    std_msgs__msg__Int32 msg;
    msg.data = 0;

    while (1) {
        rcl_publish(&publisher, &msg, NULL);
        msg.data++;
        ESP_LOGI(TAG, "Published: %d", msg.data);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void app_main(void)
{
    // 如果用 WiFi，先初始化网络
    uros_network_interface_initialize();

    xTaskCreate(micro_ros_task, "micro_ros_task", 16000, NULL, 5, NULL);
}