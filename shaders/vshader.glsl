#version 330 core

// 顶点属性
layout(location = 0) in vec3 vPosition;     // 顶点位置
layout(location = 1) in vec3 vNormal;       // 顶点法线
layout(location = 2) in vec2 vTexCoord;     // 纹理坐标
layout(location = 3) in ivec4 vBoneIndices; // 骨骼索引
layout(location = 4) in vec4 vBoneWeights;  // 骨骼权重

// 输出到 fshader
out vec3 Normal;            // 变换后的法线
out vec3 FragPos;           // 变换后的顶点位置
out vec2 UV;                // 纹理坐标
out vec4 FragPosLightSpace; // 顶点在光源空间的位置 (用于阴影)

// Uniform 变量
uniform mat4 model;            // 模型矩阵
uniform mat4 view;             // 视图矩阵
uniform mat4 projection;       // 投影矩阵
uniform mat4 lightSpaceMatrix; // 光源空间变换矩阵 (用于阴影)

// 骨骼矩阵纹理 (TBO)
uniform samplerBuffer boneMatrices;

// 从 TBO 中获取骨骼变换矩阵
mat4 getBoneMatrix(int index) {
    int baseIndex = index * 4; // 每个矩阵占 4 个像素 (RGBA32F * 4)
    vec4 c1 = texelFetch(boneMatrices, baseIndex + 0);
    vec4 c2 = texelFetch(boneMatrices, baseIndex + 1);
    vec4 c3 = texelFetch(boneMatrices, baseIndex + 2);
    vec4 c4 = texelFetch(boneMatrices, baseIndex + 3);
    return mat4(c1, c2, c3, c4);
}

void main() {
    mat4 boneTransform = mat4(0.0);
    bool hasBones = false;
    
    // 检查是否有有效的骨骼权重
    if (vBoneWeights.x + vBoneWeights.y + vBoneWeights.z + vBoneWeights.w > 0.0) {
        hasBones = true;
        // 线性混合蒙皮 (LBS)
        boneTransform += getBoneMatrix(vBoneIndices.x) * vBoneWeights.x;
        boneTransform += getBoneMatrix(vBoneIndices.y) * vBoneWeights.y;
        boneTransform += getBoneMatrix(vBoneIndices.z) * vBoneWeights.z;
        boneTransform += getBoneMatrix(vBoneIndices.w) * vBoneWeights.w;
    } else {
        // 如果没有则使用单位矩阵
        boneTransform = mat4(1.0);
    }

    vec4 pos = vec4(vPosition, 1.0);
    vec3 norm = vNormal;

    // 应用骨骼变换
    if (hasBones) {
        pos = boneTransform * pos;
        // 法线变换矩阵应该是骨骼变换矩阵的逆转置矩阵
        // 这里假设骨骼变换主要是旋转和平移，缩放为统一缩放，直接取左上 3x3 近似
        norm = mat3(boneTransform) * norm;
    }

    // 计算世界空间位置
    FragPos = vec3(model * pos);
    
    // 计算世界空间法线
    Normal = mat3(transpose(inverse(model))) * norm;
    
    UV = vTexCoord;
    
    // 计算光源空间位置
    FragPosLightSpace = lightSpaceMatrix * vec4(FragPos, 1.0);
    
    // 计算裁剪空间位置
    gl_Position = projection * view * vec4(FragPos, 1.0);
}