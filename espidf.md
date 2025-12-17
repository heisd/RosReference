# 这是一个简单使用esp.idf的参考手册+micro_ros的工具
## 1.创建工作空间
```bash
mkdir ESP32 && cd ESP32
git clone https://github.com/espressif/esp-idf.git

```
## 2.配置环境
```bash
    . ./install.sh #依赖于python-venv
    . ./export.sh
```
## 3.idf.py使用
```bash
    # 创建项目
    idf.py create-project my_micro_ros_project
    cd my_micro_ros_project
    # 克隆micro_ros库
    mkdir -p components
    cd components
    git clone https://github.com/micro-ROS/micro_ros_espidf_component.git
    cd ..
    # 注意项目要和esp-idf在同一级目录下
    # 选板 ESP32-S3
    idf.py set-target esp32s3
    # 配置
    idf.py menuconfig
    # 编译
    idf.py build
    # 烧录
    idf.py flash
    # 监控
    idf.py monitor
```

