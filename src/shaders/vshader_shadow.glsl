#version 330 core

layout(location = 0) in vec3 vPosition;     // 顶点位置
layout(location = 3) in ivec4 vBoneIndices; // 骨骼索引
layout(location = 4) in vec4 vBoneWeights;  // 骨骼权重

uniform mat4 lightSpaceMatrix; // 光源空间的变换矩阵
uniform mat4 model;            // 模型矩阵
uniform samplerBuffer boneMatrices; // 骨骼矩阵 TBO

// 获取骨骼矩阵
mat4 getBoneMatrix(int index) {
    int baseIndex = index * 4;
    vec4 c1 = texelFetch(boneMatrices, baseIndex + 0);
    vec4 c2 = texelFetch(boneMatrices, baseIndex + 1);
    vec4 c3 = texelFetch(boneMatrices, baseIndex + 2);
    vec4 c4 = texelFetch(boneMatrices, baseIndex + 3);
    return mat4(c1, c2, c3, c4);
}

void main() {
    mat4 boneTransform = mat4(0.0);
    // 计算骨骼变换
    if (vBoneWeights.x + vBoneWeights.y + vBoneWeights.z + vBoneWeights.w > 0.0) {
        boneTransform += getBoneMatrix(vBoneIndices.x) * vBoneWeights.x;
        boneTransform += getBoneMatrix(vBoneIndices.y) * vBoneWeights.y;
        boneTransform += getBoneMatrix(vBoneIndices.z) * vBoneWeights.z;
        boneTransform += getBoneMatrix(vBoneIndices.w) * vBoneWeights.w;
    } else {
        boneTransform = mat4(1.0);
    }

    // 应用骨骼变换
    vec4 pos = boneTransform * vec4(vPosition, 1.0);
    
    // 变换到光源空间的裁剪坐标系
    gl_Position = lightSpaceMatrix * model * pos;
}
