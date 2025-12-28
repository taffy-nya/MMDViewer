#version 330 core

// 顶点属性
layout(location = 0) in vec3 vPosition;     // 顶点位置
layout(location = 1) in vec3 vNormal;       // 顶点法线
layout(location = 3) in ivec4 vBoneIndices; // 骨骼索引
layout(location = 4) in vec4 vBoneWeights;  // 骨骼权重

// Uniform 变量
uniform mat4 model;      // 模型矩阵
uniform mat4 view;       // 视图矩阵
uniform mat4 projection; // 投影矩阵
uniform float edge_size; // 描边粗细

// 骨骼矩阵纹理 (TBO)
uniform samplerBuffer boneMatrices;

// 输出到 fshader
out vec3 FragPos;
out vec3 Normal;

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
    bool hasBones = false;
    
    // 骨骼蒙皮计算
    if (vBoneWeights.x + vBoneWeights.y + vBoneWeights.z + vBoneWeights.w > 0.0) {
        hasBones = true;
        boneTransform += getBoneMatrix(vBoneIndices.x) * vBoneWeights.x;
        boneTransform += getBoneMatrix(vBoneIndices.y) * vBoneWeights.y;
        boneTransform += getBoneMatrix(vBoneIndices.z) * vBoneWeights.z;
        boneTransform += getBoneMatrix(vBoneIndices.w) * vBoneWeights.w;
    } else {
        boneTransform = mat4(1.0);
    }

    vec4 pos = vec4(vPosition, 1.0);
    vec3 norm = vNormal;

    if (hasBones) {
        pos = boneTransform * pos;
        norm = mat3(boneTransform) * norm;
    }

    // 沿法线方向挤出顶点，形成一个比原模型稍大的轮廓，就是描边
    vec3 pos3 = pos.xyz + norm * edge_size;
    
    FragPos = vec3(model * vec4(pos3, 1.0));
    Normal = mat3(transpose(inverse(model))) * norm;

    gl_Position = projection * view * model * vec4(pos3, 1.0);
}