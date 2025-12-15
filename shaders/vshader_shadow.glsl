#version 330 core
layout(location = 0) in vec3 vPosition;
layout(location = 3) in ivec4 vBoneIndices;
layout(location = 4) in vec4 vBoneWeights;

uniform mat4 lightSpaceMatrix;
uniform mat4 model;
uniform samplerBuffer boneMatrices;

mat4 getBoneMatrix(int index) {
    int baseIndex = index * 4;
    vec4 c1 = texelFetch(boneMatrices, baseIndex + 0);
    vec4 c2 = texelFetch(boneMatrices, baseIndex + 1);
    vec4 c3 = texelFetch(boneMatrices, baseIndex + 2);
    vec4 c4 = texelFetch(boneMatrices, baseIndex + 3);
    return mat4(c1, c2, c3, c4);
}

void main()
{
    mat4 boneTransform = mat4(0.0);
    if (vBoneWeights.x + vBoneWeights.y + vBoneWeights.z + vBoneWeights.w > 0.0) {
        boneTransform += getBoneMatrix(vBoneIndices.x) * vBoneWeights.x;
        boneTransform += getBoneMatrix(vBoneIndices.y) * vBoneWeights.y;
        boneTransform += getBoneMatrix(vBoneIndices.z) * vBoneWeights.z;
        boneTransform += getBoneMatrix(vBoneIndices.w) * vBoneWeights.w;
    } else {
        boneTransform = mat4(1.0);
    }

    vec4 pos = boneTransform * vec4(vPosition, 1.0);
    gl_Position = lightSpaceMatrix * model * pos;
}
