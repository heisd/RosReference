# 修改CMake文件在install目录下添加
```cmake
install(DIRECTORY src urdf meshes include share/{PROJECT_NAME})
```

```bash
ls -R install/visual_robot/share/visual_robot/meshes
```
只要这个有输出就可以使用moveit_setup_assistant啦


