#version 330 core
layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;

out vec3 Normal;
out vec3 FragPos;
out vec4 FragPosLightSpace;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceMatrix;

void main() {
    // 计算世界空间中的顶点位置
    FragPos = vec3(model * vec4(vPosition, 1.0));
    
    // 计算变换后的法线 (使用法线矩阵来处理非均匀缩放)
    Normal = mat3(transpose(inverse(model))) * vNormal;  
    
    // 将顶点位置变换到光空间
    FragPosLightSpace = lightSpaceMatrix * vec4(FragPos, 1.0);
    
    // 计算裁剪空间中的顶点位置
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
