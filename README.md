# MMDViewer

这是小菲的计算机图形学期末大作业，是一个 OpenGL 的 MMD 模型与动作查看器

# 功能

- 模型
    - PMX 模型文件的部分解析，包括顶点面片、纹理、材质、部分骨骼等，其它的例如变形（表情）、刚体等暂未实现
    - 基于 Toon Shader 的二次元风格渲染

- 动作与动画
    - VMD 动作文件解析
    - FK 与基于 CCD 算法的 IK 骨骼变形
    - 骨骼可视化与手动控制
    - 基于线性插值的动画过渡

- 光照与阴影
    - Blinn-Phong 光照模型
    - 平行光与点光源的多光源支持
    - 基于深度贴图的单光源实时阴影渲染

- UI 交互
    - 基于 ImGui 的操作面板，可以通过面板加载模型、控制动画播放、控制骨骼、控制光源等
    - 集成 ImGuizmo 库从而更直观地控制节点的平移与旋转

# 项目结构

```
MMDViewer/
├── MMDViewer.cpp                   
├── Camera.cpp
├── MeshPainter.cpp 
├── TriMesh.cpp
├── VMDAnimation.cpp 
├── Stage.cpp 
├── InitShader.cpp 
├── glad.c 
├── include/ 
│   ├── Camera.h, TriMesh.h, ...
├── shaders/ 
│   ├── vshader_*.glsl,…
│   └── fshader_*.glsl,…
├── models/
├── motions/
└── stages/
```

- `MMDViewer.cpp`: 主程序入口，负责窗口管理、键鼠控制和 GUI 逻辑
- `Camera.cpp`: 相机类实现 
- `MeshPainter.cpp`: 渲染组织层，负责设置渲染状态并下发 OpenGL 绘制指令
- `TriMesh.cpp`: PMX 模型解析与 Mesh 数据管理
- `VMDAnimation.cpp`: VMD 动作解析、关键帧插值与骨骼姿态计算 
- `Stage.cpp`: 场景模型管理 
- `InitShader.cpp`: 着色器编译、链接管理
- `glad.c`: OpenGL 函数加载实现
- `include/`: 头文件集合
- `shaders/`: 着色器集合
- `models/`: PMX 模型资源
- `motions/`: VMD 动作资源
- `stages/`: PMX 场景资源

```
shaders/
├── vshader_with_light.glsl
├── fshader_with_light.glsl
├── vshader_edge.glsl
├── fshader_edge.glsl
├── vshader_shadow.glsl
├── fshader_shadow.glsl
├── vshader_stage.glsl
├── fshader_stage.glsl
├── vshader_color.glsl
└── fshader_color.glsl
```

- `vshader_with_light.glsl`: 模型主体渲染，处理骨骼蒙皮、坐标变换
- `fshader_with_light.glsl`: 模型主体渲染，计算光照、阴影、Toon 材质
- `vshader_edge.glsl`: 描边渲染，将顶点延法向挤出形成描边
- `fshader_edge.glsl`: 描边渲染，输出描边颜色
- `vshader_shadow.glsl`: 阴影生成，变换至光源空间
- `fshader_shadow.glsl`: 阴影生成，输出深度值
- `vshader_stage.glsl`: 场景渲染，处理静态场景变换
- `fshader_stage.glsl`: 场景渲染，渲染静态场景材质
- `vshader_color.glsl`: 用于绘制线框、gizmo 等
- `fshader_color.glsl`: 输出纯色

# 编译

clone 仓库后用 CMake 构建：

```bash
git clone https://github.com/taffy-nya/MMDViewer.git && cd MMDViewer
cmake -B build
cmake --build build
```

# 已知的问题

- [ ] 角色模型的眼睛会产生错误的阴影
