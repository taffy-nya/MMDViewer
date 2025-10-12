#version 330 core
layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float edge_size;

void main()
{
    vec4 pos = vec4(vPosition + vNormal * edge_size, 1.0);
    gl_Position = projection * view * model * pos;
}