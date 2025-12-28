// shaders/fshader_debug_uv.glsl
#version 330 core

out vec4 fColor;
in vec2 UV; // 从顶点着色器传入的 UV 坐标

void main() {
    // 将 UV 坐标直接作为颜色输出
    // U -> R (红色通道)
    // V -> G (绿色通道)
    // B (蓝色通道) 设置为 0.0
    fColor = vec4(UV.x, UV.y, 0.0, 1.0);
}