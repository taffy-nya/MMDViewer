#version 330 core
out vec4 fColor;

uniform vec4 edge_color;

void main()
{
    fColor = edge_color;
}