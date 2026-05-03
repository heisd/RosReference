# MoveIt 功能包资源文件安装修复

## 问题描述

使用 MoveIt Setup Assistant 加载自定义功能包的 URDF 时，如果功能包的 `meshes`、`urdf`、`include` 等目录没有被 CMake 正确安装到 `share` 目录，会导致以下问题：

- Setup Assistant 加载 URDF 时找不到 mesh 文件，模型显示异常或加载失败
- `rviz2` 显示机械臂时连杆形状缺失（显示为白色方块）
- `robot_state_publisher` 加载 URDF 报路径错误

## 解决方案

在功能包的 `CMakeLists.txt` 末尾（`ament_package()` 之前）添加以下 `install` 指令，将资源目录安装到 `share` 路径下：

```cmake
install(
    DIRECTORY src urdf meshes include
    DESTINATION share/${PROJECT_NAME}
)
```

> 若功能包中不存在某个目录（如没有 `src` 目录），CMake 不会报错，可直接保留该行。

## 完整 CMakeLists.txt 示例

```cmake
cmake_minimum_required(VERSION 3.8)
project(visual_robot)

find_package(ament_cmake REQUIRED)
find_package(urdf REQUIRED)
find_package(xacro REQUIRED)

# 安装资源文件到 share 目录
install(
    DIRECTORY src urdf meshes include
    DESTINATION share/${PROJECT_NAME}
)

ament_package()
```

## 验证修复

重新编译并 source 工作空间，运行以下命令检查 meshes 是否被正确安装：

```bash
colcon build --packages-select visual_robot
source install/setup.zsh

# 检查 meshes 目录是否存在
ls -R install/visual_robot/share/visual_robot/meshes
```

若命令有正常输出（列出 mesh 文件），则修复成功，可以正常使用 MoveIt Setup Assistant 加载该功能包。

## 注意事项

- 每次修改 `CMakeLists.txt` 后都需要重新执行 `colcon build`。
- 若使用 `--symlink-install` 编译，修改 URDF/mesh 文件后无需重新编译，但新增文件仍需重新 build。
- 路径区分大小写，确保 `meshes` 目录名与 URDF 文件中的引用路径大小写一致。
