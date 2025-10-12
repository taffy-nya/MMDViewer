#version 330 core

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec2 vUV; // 顶点UV坐标

out vec3 FragPos;
out vec3 Normal;
out vec2 UV; // 传递给片元着色器的UV

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    FragPos = vec3(model * vec4(vPosition, 1.0));
    Normal = mat3(transpose(inverse(model))) * vNormal;
    gl_Position = projection * view * vec4(FragPos, 1.0);

    UV = vUV; // 直接传递UV坐标
}