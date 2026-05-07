# MoveIt2 配置与使用指南（基于 Panda 机械臂）

> 本文记录了使用 MoveIt Setup Assistant 对 Franka Panda 机械臂进行完整配置的流程，以及在 WSL2 环境下排查 MoveIt 启动问题的经历。  
> 参考链接：<https://zhuanlan.zhihu.com/p/1920253906883187153>

---

## 一、准备工作

### 1.1 安装 MoveIt Setup Assistant

```bash
sudo apt update
sudo apt install ros-humble-moveit-setup-assistant
```

### 1.2 获取机械臂 URDF（两种方案）

#### 方案 A：使用 UR5e 工业机械臂描述包

```bash
# 克隆 UR 描述包
git clone -b humble https://github.com/UniversalRobots/Universal_Robots_ROS2_Description.git

# 编译描述包
cd Universal_Robots_ROS2_Description
colcon build --packages-select ur_description
source install/setup.zsh

# 生成静态 URDF 文件（必须禁用 ros2_control，否则加载时 MoveIt 会崩溃）
cd install/ur_description/share/ur_description
xacro urdf/ur.urdf.xacro ur_type:=ur5e name:=ur generate_ros2_control_tag:=false > ur5e_no_control.urdf
```

#### 方案 B：使用 MoveIt 官方 Panda 资源包（推荐，WSL2 环境更稳定）

```bash
mkdir ~/moveit_ws && cd ~/moveit_ws
git clone -b humble https://github.com/ros-planning/moveit_resources.git src/moveit_resources
colcon build --symlink-install
source install/setup.zsh
```

URDF 路径：`~/moveit_ws/src/moveit_resources/panda_description/urdf/panda.urdf`

---

## 二、WSL2 环境问题排查

> 在 WSL2 中直接运行 MoveIt Setup Assistant 时，常因 OpenGL / Qt 显示问题导致加载到 70% 时崩溃。按以下步骤解决。

### 问题：加载 URDF 时程序崩溃（exit code -11）

错误信息：

```
[ERROR] [moveit_setup_assistant-1]: process has died [pid 1357, exit code -11]
```

**根本原因：** MobaXterm 的 X Server 与 WSL2 的 Qt 平台插件冲突，导致 GPU 渲染异常。

**解决方法：** 在 `~/.zshrc`（或 `~/.bashrc`）末尾添加以下环境变量，强制 Qt 使用 XCB 平台：

```bash
export QT_QPA_PLATFORM=xcb
```

添加后执行 `source ~/.zshrc` 使其生效，然后重新启动 Setup Assistant。

### 编译速度过慢

WSL2 编译 MoveIt 相关包速度很慢，可通过扩展虚拟内存解决，参考 [wsl.md](./wsl.md)。

---

## 三、使用 MoveIt Setup Assistant 配置 Panda

### 启动工具

```bash
cd ~/moveit_ws
source install/setup.zsh
ros2 launch moveit_setup_assistant setup_assistant.launch.py
```

### 步骤 1：加载 URDF 文件

启动后弹出欢迎界面，点击「Create New MoveIt Configuration Package」，选择 URDF 文件路径：

```
~/moveit_ws/src/moveit_resources/panda_description/urdf/panda.urdf
```

点击「Load Files」，界面如下：

![选择URDF](./picture/moveit/moveit.png)

加载成功后进入配置主界面：

![加载成功](./picture/moveit/moveit2.png)

### 步骤 2：生成碰撞矩阵（Self Collisions）

点击左侧「Self Collisions」选项卡，点击「Generate Collision Matrix」按钮，自动计算机械臂各连杆间的碰撞关系，用于规划时排除自碰撞检测。

![碰撞矩阵](./picture/moveit/moveit3.png)

### 步骤 3：添加虚拟关节（Virtual Joints）

虚拟关节用于将机器人基座与世界坐标系连接，分两种情况：

- **固定底座机器人（如 Panda）：** 可选择设置一个 fixed 类型的虚拟关节，也可跳过。
- **移动底座机器人（如移动小车上的机械臂）：** 需添加 planar 或 floating 类型虚拟关节，设置如下：

![虚拟关节](./picture/moveit/virtual.png)

点击「Save」进入下一步。

### 步骤 4：添加规划组（Planning Groups）

规划组定义了 MoveIt 进行运动规划的关节集合。Panda 需要添加两个规划组：`panda_arm` 和 `hand`。

点击「Add Group」，有两种添加方式：

#### 添加运动链（Add Kin. Chain）

适用于连续的串联机械臂，指定从 Base Link 到 Tip Link 的完整运动链。MoveIt 会自动识别中间所有关节。用于配置 `panda_arm`：

![规划组配置](./picture/moveit/planningGroup.png)

#### 添加关节（Add Joints）

适用于需要单独选取某些关节的场景，如夹爪。手动勾选所需关节，用于配置 `hand`：

![添加关节](./picture/moveit/PlanningGroup1.png)

配置完成后结果如下：

![规划组结果](./picture/moveit/PlanningGroup2.png)

### 步骤 5：添加机器人预设姿势（Robot Poses）

预设姿势可在后续通过 MoveIt API 直接调用，例如"零位姿""收拢姿势"等。

#### panda_arm 预设姿势

![arm 姿势](./picture/moveit/RobotPose1.png)

#### hand 预设姿势

![hand 姿势](./picture/moveit/RobotPose2.png)

### 步骤 6：标记末端执行器（End Effectors）

将 `hand` 规划组标记为末端执行器，便于 MoveIt 在规划时处理抓取任务：

![末端执行器](./picture/moveit/EndEffectors.png)

### 步骤 7：被动关节（Passive Joints）

被动关节是无法主动驱动的关节，需要声明以避免规划器尝试对其规划。Panda 机械臂没有被动关节，**跳过此步骤**。

### 步骤 8：修改 ROS2 Control URDF

此步骤提供一个编辑框，展示自动生成的 `ros2_control` URDF 片段，通常无需手动修改，直接继续即可。

### 步骤 9：配置 ROS2 Controllers

为 `panda_arm` 和 `hand` 分别添加控制器：

![ROS2Controller1](./picture/moveit/ROS2Controller1.png)
![ROS2Controller2](./picture/moveit/ROS2Controller2.png)
![ROS2Controller3](./picture/moveit/ROS2Controller3.png)

为 `hand` 同样配置控制器后，结果如下：

![ROS2Controller4](./picture/moveit/ROS2Controller4.png)

### 步骤 10：配置 MoveIt Controllers

为 `panda_arm` 添加 MoveIt 控制器：

![MoveitController1](./picture/moveit/MoveitController1.png)

为 `hand` 同样添加：

![MoveitController2](./picture/moveit/MoveitController2.png)

### 步骤 11：配置 3D 感知传感器（可选）

「Perception」选项卡用于配置深度相机或激光雷达，生成 `sensors_3d.yaml`。  
若不需要 3D 感知，选择「None」跳过。

配置点云示例如下：

![3D传感器](./picture/moveit/3DPerceptionSensors.png)

### 步骤 12：生成配置包

在「Configuration Files」选项卡中，设置输出路径：

```
/home/li/moveit_ws/src/panda_moveit_config
```

点击「Generate Package」：

![生成配置文件](./picture/moveit/GenerateConfigurationFiles.png)

---

## 四、编译并运行配置包

### 4.1 编译

```bash
cd ~/moveit_ws
colcon build --packages-select panda_moveit_config
source install/setup.zsh
```

### 4.2 安装缺失依赖

首次运行若报以下错误：

```
[ERROR] package 'controller_manager' not found
```

安装缺失包：

```bash
sudo apt update
sudo apt install ros-humble-controller-manager \
                 ros-humble-ros2-control \
                 ros-humble-ros2-controllers
```

### 4.3 启动 Demo

```bash
ros2 launch panda_moveit_config demo.launch.py
```

启动成功后 RViz2 展示结果如下，可在界面中拖动末端执行器进行运动规划：

![RViz2结果](./picture/moveit/RVIZ2Result.png)

至此，Panda 机械臂的 MoveIt2 配置包已完整生成并可正常使用。

---

## 五、通过 MoveIt Python API 控制机械臂（进阶）

配置包就绪后，可以通过 MoveIt Python API 编程控制机械臂。

### 5.1 安装 MoveIt Python 接口

```bash
sudo apt install ros-humble-moveit-py
```

### 5.2 最小控制示例

```python
import rclpy
from rclpy.node import Node
from moveit.planning import MoveItPy
from moveit.core.robot_state import RobotState


def main():
    rclpy.init()
    node = rclpy.create_node("panda_motion_node")

    # 初始化 MoveItPy，参数与 demo.launch.py 保持一致
    panda = MoveItPy(node_name="moveit_py")
    arm = panda.get_planning_component("panda_arm")

    # 规划并执行到预设姿势 "ready"
    arm.set_start_state_to_current_state()
    arm.set_goal_state(configuration_name="ready")
    plan_result = arm.plan()

    if plan_result:
        panda.execute(plan_result.trajectory, controllers=[])
    else:
        node.get_logger().error("规划失败")

    rclpy.shutdown()


if __name__ == "__main__":
    main()
```

### 5.3 运行

确保 `demo.launch.py` 已在另一个终端运行，然后执行：

```bash
python3 panda_motion.py
```

机械臂将自动运动到"ready"预设姿势。
