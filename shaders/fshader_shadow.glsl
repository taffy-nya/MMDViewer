#version 330 core

// 阴影生成的 fshader
// 不需要输出颜色，只需要深度信息

void main() {
    // OpenGL 会自动将深度值 gl_FragDepth 写入深度缓冲，所以 main 函数什么都不用做
}
