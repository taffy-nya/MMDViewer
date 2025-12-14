#pragma once
#include <string>
#include <unordered_map>

inline std::string get_embedded_shader(const std::string& filename) {
    static std::unordered_map<std::string, std::string> shaders = {
        {"shaders/fshader.glsl", R"(#version 330 core
out vec4 fColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 UV;

uniform vec4 objectColor;   
uniform vec3 lightColor;
uniform vec3 lightPos;
uniform vec3 viewPos;

uniform sampler2D textureSampler;
uniform bool hasTexture;
uniform sampler2D toonSampler;

void main()
{
    vec4 textureColor = vec4(1.0);
    if (hasTexture) {
        textureColor = texture(textureSampler, UV);
    }
    // 只使用物体颜色和贴图，不做任何光照
    vec4 finalColor = objectColor * textureColor;
    fColor = vec4(finalColor.rgb, finalColor.a);
})"},
        {"shaders/fshader_debug_uv.glsl", R"(// shaders/fshader_debug_uv.glsl
#version 330 core

out vec4 fColor;
in vec2 UV; // 从顶点着色器传入的 UV 坐标

void main()
{
    // 将 UV 坐标直接作为颜色输出
    // U -> R (红色通道)
    // V -> G (绿色通道)
    // B (蓝色通道) 设置为 0.0
    fColor = vec4(UV.x, UV.y, 0.0, 1.0);
})"},
        {"shaders/fshader_edge.glsl", R"(#version 330 core
out vec4 fColor;

uniform vec4 edge_color;

void main()
{
    fColor = edge_color;
})"},
        {"shaders/fshader_with_light.glsl", R"(#version 330 core
out vec4 fColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 UV;

uniform vec4 objectColor;   
uniform vec3 lightColor;
uniform vec3 lightPos;
uniform vec3 viewPos;
uniform float brightness;

uniform sampler2D textureSampler;
uniform bool hasTexture;
uniform sampler2D toonSampler;

void main()
{
    vec4 textureColor = vec4(1.0);
    if (hasTexture) {
        textureColor = texture(textureSampler, UV);
    }
    
    vec3 finalObjectColor = objectColor.rgb * textureColor.rgb;

    // --- Toon Shading Light Calculation ---
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    
    float lightIntensity = dot(norm, lightDir) * 0.5 + 0.5;
    vec3 toonColor = texture(toonSampler, vec2(0.5, lightIntensity)).rgb;

    float ambientStrength = 0.4; // MMD ambient is usually stronger
    vec3 ambient = ambientStrength * lightColor;

    vec3 diffuse = toonColor * lightColor;
    
    // Specular can be kept for highlights
    float specularStrength = 0.5;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(norm, halfwayDir), 0.0), 32.0);
    vec3 specular = specularStrength * spec * lightColor * objectColor.rgb;

    // --- Final Color ---
    vec3 result = (ambient + diffuse) * finalObjectColor + specular;
    
    fColor = vec4(result * brightness, textureColor.a * objectColor.a);
})"},
        {"shaders/vshader.glsl", R"(#version 330 core

layout(location = 0) in vec3 vPosition;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec2 vTexCoord;
layout(location = 3) in ivec4 vBoneIndices;
layout(location = 4) in vec4 vBoneWeights;

out vec3 Normal;
out vec3 FragPos;
out vec2 UV;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

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
    bool hasBones = false;
    
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

    FragPos = vec3(model * pos);
    Normal = mat3(transpose(inverse(model))) * norm;
    UV = vTexCoord;
    
    gl_Position = projection * view * vec4(FragPos, 1.0);
})"},
        {"shaders/vshader_edge.glsl", R"(#version 330 core

layout(location = 0) in vec3 vPosition;
layout(location = 1) in vec3 vNormal;
layout(location = 3) in ivec4 vBoneIndices;
layout(location = 4) in vec4 vBoneWeights;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float edge_size;

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
    bool hasBones = false;
    
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

    // Extrude along normal
    vec3 pos3 = pos.xyz + norm * edge_size;
    
    gl_Position = projection * view * model * vec4(pos3, 1.0);
})"},
    };
    auto it = shaders.find(filename);
    if (it != shaders.end()) return it->second;
    return "";
}
