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
        // 采样纹理颜色
        textureColor = texture(textureSampler, UV);
    }
    // 只使用物体颜色和贴图，不做任何光照计算（Unlit Shader）
    vec4 finalColor = objectColor * textureColor;
    fColor = vec4(finalColor.rgb, finalColor.a);
})"},
        {"shaders/fshader_color.glsl", R"(#version 330 core
out vec4 FragColor;

// 统一颜色输入
uniform vec3 color;

void main() {
    // 输出纯色（Alpha 设为 1.0）
    FragColor = vec4(color, 1.0);
})"},
        {"shaders/fshader_debug_uv.glsl", R"(// shaders/fshader_debug_uv.glsl
#version 330 core

out vec4 fColor;
in vec2 UV; // 从顶点着色器传入的 UV 坐标

void main() {
    // 将 UV 坐标直接作为颜色输出
    // U -> R (红色通道)
    // V -> G (绿色通道)
    // B (蓝色通道) 设置为 0.0
    fColor = vec4(UV.x, UV.y, 0.0, 1.0);
})"},
        {"shaders/fshader_edge.glsl", R"(#version 330 core
out vec4 fColor;

in vec3 FragPos;
in vec3 Normal;

uniform vec4 edge_color;

// 光源，与 Light.h 相同
struct Light {
    vec3 position;
    vec3 direction;
    vec3 color;
    float intensity;
    int type;
    float constant;
    float linear;
    float quadratic;
    bool enabled;
};

#define MAX_LIGHTS 16
uniform Light lights[MAX_LIGHTS];
uniform int numLights;
uniform vec3 ambientColor;
uniform float ambientStrength;

void main() {
    vec3 norm = normalize(Normal);
    vec3 lighting = vec3(0.0);
    
    // 描边光照计算，只计算漫反射
    for(int i = 0; i < numLights; i++) {
        if(!lights[i].enabled) continue;
        
        vec3 lightDir;
        float attenuation = 1.0;

        if (lights[i].type == 0) { // 平行光
            lightDir = normalize(-lights[i].direction);
        } else { // 点光源
            lightDir = normalize(lights[i].position - FragPos);
            float distance = length(lights[i].position - FragPos);
            attenuation = 1.0 / (lights[i].constant + lights[i].linear * distance + lights[i].quadratic * (distance * distance));
        }

        float diff = max(dot(norm, lightDir), 0.0);
        lighting += diff * lights[i].color * lights[i].intensity * attenuation;
    }

    vec3 ambient = ambientStrength * ambientColor;
    // 最终颜色 = (环境光 + 漫反射) * 描边颜色
    vec3 result = (ambient + lighting) * edge_color.rgb;
    
    // 保持 Alpha 通道
    fColor = vec4(result, edge_color.a);
})"},
        {"shaders/fshader_shadow.glsl", R"(#version 330 core

// 阴影生成的 fshader
// 不需要输出颜色，只需要深度信息

void main() {
    // OpenGL 会自动将深度值 gl_FragDepth 写入深度缓冲，所以 main 函数什么都不用做
}
)"},
        {"shaders/fshader_stage.glsl", R"(#version 330 core
out vec4 FragColor;

in vec3 Normal;
in vec3 FragPos;
in vec4 FragPosLightSpace;

// 光源，与 Light.h 相同
struct Light {
    vec3 position;
    vec3 direction;
    vec3 color;
    float intensity;
    int type;
    float constant;
    float linear;
    float quadratic;
    bool enabled;
};

uniform Light lights[16];   // 光源数组，最多 16 个光源
uniform int numLights;      // 实际光源数量

uniform vec4 objectColor;   // 物体颜色
uniform vec3 viewPos;       // 相机位置
uniform vec3 ambientColor;  // 环境光颜色
uniform float ambientStrength; // 环境光强度
uniform sampler2DShadow shadowMap;  // 阴影贴图

// 阴影计算函数
// fragPosLightSpace 为光空间中 frag 的位置
float ShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir) {
    // 透视除法
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;

    // 变换到 [0,1] 范围
    projCoords = projCoords * 0.5 + 0.5;

    // 获取当前片段的深度
    float currentDepth = projCoords.z;

    // 计算阴影偏移, 防止阴影失真
    float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.0005);
    
    float shadow = 0.0;
    // PCF 柔和阴影
    // 对周围 3*3 区域进行采样并取平均值，使阴影边缘更柔和
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            // texture(sampler2DShadow) 返回可见度 (1.0 完全可见，0.0 完全遮挡)
            float pcfDepth = texture(shadowMap, vec3(projCoords.xy + vec2(x, y) * texelSize, currentDepth - bias)); 
            shadow += (1.0 - pcfDepth); // 累加遮挡度
        }    
    }
    shadow /= 9.0; // 取平均
    
    // 如果超出远平面，则没有阴影
    if(projCoords.z > 1.0)
        shadow = 0.0;
        
    return shadow;
}

void main() {
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);
    
    // 环境光
    vec3 ambient = ambientStrength * ambientColor * objectColor.rgb;
    
    vec3 lighting = vec3(0.0);
    
    // 遍历所有光源
    for (int i = 0; i < numLights; i++) {
        if (!lights[i].enabled) continue;
        
        vec3 lightDir;
        float attenuation = 1.0;
        
        if (lights[i].type == 0) { // 平行光
             lightDir = normalize(-lights[i].direction);
        } else { // 点光源
             lightDir = normalize(lights[i].position - FragPos);
             float distance = length(lights[i].position - FragPos);
             // 计算衰减
             attenuation = 1.0 / (lights[i].constant + lights[i].linear * distance + lights[i].quadratic * (distance * distance));
        }
        
        // 漫反射
        float diff = max(dot(norm, lightDir), 0.0);
        
        // 计算阴影 (仅对第一个平行光计算阴影)
        float shadow = 0.0;
        if (i == 0 && lights[i].type == 0) {
            shadow = ShadowCalculation(FragPosLightSpace, norm, lightDir);
        }
        
        vec3 diffuse = diff * lights[i].color * lights[i].intensity * objectColor.rgb * (1.0 - shadow);
        
        // Blinn-Phong 模型的镜面反射
        vec3 halfwayDir = normalize(lightDir + viewDir);
        float spec = pow(max(dot(norm, halfwayDir), 0.0), 32.0);
        vec3 specular = 0.5 * spec * lights[i].color * lights[i].intensity * (1.0 - shadow);
        
        // 累加光照结果
        lighting += (diffuse + specular) * attenuation;
    }
    
    // 最终颜色 = 环境光 + 光照累加
    vec3 result = ambient + lighting;
    FragColor = vec4(result, objectColor.a);
}
)"},
        {"shaders/fshader_with_light.glsl", R"(#version 330 core
out vec4 fColor;

in vec3 FragPos;           // 世界空间位置
in vec3 Normal;            // 世界空间法线
in vec2 UV;                // 纹理坐标
in vec4 FragPosLightSpace; // 光源空间位置

// 光源，与 Light.h 的一致
struct Light {
    vec3 position;
    vec3 direction;
    vec3 color;
    float intensity;
    int type;
    float constant;
    float linear;
    float quadratic;
    bool enabled;
};

uniform Light lights[16];       // 光源数组
uniform int numLights;          // 实际光源数量

// 材质属性
uniform vec4 objectColor;       // 漫反射颜色
uniform vec3 viewPos;           // 相机位置
uniform float brightness;       // 全局亮度 (未使用)
uniform float shininess;        // 高光反光度
uniform vec3 specularColor;     // 高光颜色

// 环境光
uniform vec3 ambientColor;
uniform float ambientStrength;

// 纹理采样器
uniform sampler2D textureSampler; // 基础纹理
uniform bool hasTexture;          // 是否有基础纹理
uniform sampler2D toonSampler;    // Toon 渐变纹理
uniform sampler2DShadow shadowMap;// 阴影深度贴图

// 阴影计算函数
// fragPosLightSpace 为光空间中 frag 的位置
float ShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir){
    // 透视除法
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // 变换到 [0,1] 范围
    projCoords = projCoords * 0.5 + 0.5;
    
    // 获取当前片段的深度值
    float currentDepth = projCoords.z;
    
    // 计算阴影偏移, 防止阴影失真
    float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.0005);
    
    float shadow = 0.0;
    // PCF 柔和阴影
    // 对周围 3*3 区域进行采样并取平均值，使阴影边缘更柔和
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            // texture(sampler2DShadow) 返回可见度 (1.0 完全可见，0.0 完全遮挡)
            float pcfDepth = texture(shadowMap, vec3(projCoords.xy + vec2(x, y) * texelSize, currentDepth - bias)); 
            shadow += (1.0 - pcfDepth); // 累加遮挡度
        }    
    }
    shadow /= 9.0; // 取平均
    
    // 如果超过远平面，则没有阴影
    if(projCoords.z > 1.0)
        shadow = 0.0;
        
    return shadow;
}

void main() {
    // 采样基础纹理
    vec4 textureColor = vec4(1.0);
    if (hasTexture) {
        textureColor = texture(textureSampler, UV);
    }
    
    // 最终物体颜色 = 材质颜色 * 纹理颜色
    vec3 finalObjectColor = objectColor.rgb * textureColor.rgb;
    
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);

    // 环境光 (Ambient)
    vec3 ambient = ambientStrength * ambientColor * finalObjectColor;

    vec3 lighting = vec3(0.0);

    for(int i = 0; i < numLights; i++) {
        if(!lights[i].enabled) continue;
        
        vec3 lightDir;
        float attenuation = 1.0;

        if (lights[i].type == 0) { // 方向光
             lightDir = normalize(-lights[i].direction);
        } else { // 点光源
             lightDir = normalize(lights[i].position - FragPos);
             float distance = length(lights[i].position - FragPos);
             // 物理衰减公式
             attenuation = 1.0 / (lights[i].constant + lights[i].linear * distance + lights[i].quadratic * (distance * distance));
        }

        // 漫反射 (Diffuse) - 使用 Toon Shading (卡通渲染)
        float dotProd = dot(norm, lightDir);
        // 将 [-1, 1] 映射到 [0, 1] 用于采样 Toon 纹理
        float lightIntensity = dotProd * 0.5 + 0.5;
        // 防止采样越界
        lightIntensity = clamp(lightIntensity, 0.01, 0.99); 
        // 采样 Toon 纹理获取阶梯状的光照颜色
        vec3 toonColor = texture(toonSampler, vec2(0.5, lightIntensity)).rgb;
        
        float shadow = 0.0;
        // 仅对第一个方向光计算阴影 (通常是主光源/太阳)
        if (i == 0 && lights[i].type == 0) { 
             shadow = ShadowCalculation(FragPosLightSpace, norm, lightDir);
        }

        // 漫反射贡献 = Toon颜色 * 光源颜色 * 强度 * (1 - 阴影)
        vec3 diffuse = toonColor * lights[i].color * lights[i].intensity * (1.0 - shadow);

        // 高光 (Specular) - Blinn-Phong 模型
        vec3 halfwayDir = normalize(lightDir + viewDir);
        float spec = pow(max(dot(norm, halfwayDir), 0.0), shininess);
        vec3 specular = spec * lights[i].color * lights[i].intensity * specularColor * (1.0 - shadow); 

        // 累加光照结果 (应用衰减)
        lighting += (diffuse * finalObjectColor + specular) * attenuation;
    }
    
    vec3 result = ambient + lighting;
    
    // 如果没有光源且环境光很弱，保留物体本色 (Fallback)
    if (numLights == 0 && ambientStrength < 0.01) {
         result = finalObjectColor;
    }

    fColor = vec4(result, textureColor.a * objectColor.a);
})"},
        {"shaders/vshader.glsl", R"(#version 330 core

// 顶点属性输入
layout(location = 0) in vec3 vPosition;     // 顶点位置
layout(location = 1) in vec3 vNormal;       // 顶点法线
layout(location = 2) in vec2 vTexCoord;     // 纹理坐标 (UV)
layout(location = 3) in ivec4 vBoneIndices; // 骨骼索引 (最多影响顶点的4个骨骼)
layout(location = 4) in vec4 vBoneWeights;  // 骨骼权重 (对应上述4个骨骼的权重)

// 输出到片段着色器
out vec3 Normal;            // 变换后的法线 (世界空间)
out vec3 FragPos;           // 变换后的顶点位置 (世界空间)
out vec2 UV;                // 纹理坐标
out vec4 FragPosLightSpace; // 顶点在光源空间的位置 (用于阴影计算)

// Uniform 变量
uniform mat4 model;            // 模型矩阵
uniform mat4 view;             // 视图矩阵
uniform mat4 projection;       // 投影矩阵
uniform mat4 lightSpaceMatrix; // 光源空间变换矩阵 (用于阴影)

// 骨骼矩阵纹理 (TBO)
// 使用 samplerBuffer 访问存储在纹理缓冲区中的骨骼矩阵数组
uniform samplerBuffer boneMatrices;

// 从 TBO 中获取指定索引的骨骼变换矩阵
mat4 getBoneMatrix(int index) {
    int baseIndex = index * 4; // 每个矩阵占 4 个像素 (RGBA32F * 4)
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
    
    // 检查是否有有效的骨骼权重
    if (vBoneWeights.x + vBoneWeights.y + vBoneWeights.z + vBoneWeights.w > 0.0) {
        hasBones = true;
        // 线性混合蒙皮 (Linear Blend Skinning - LBS)
        // 累加加权后的骨骼变换矩阵
        boneTransform += getBoneMatrix(vBoneIndices.x) * vBoneWeights.x;
        boneTransform += getBoneMatrix(vBoneIndices.y) * vBoneWeights.y;
        boneTransform += getBoneMatrix(vBoneIndices.z) * vBoneWeights.z;
        boneTransform += getBoneMatrix(vBoneIndices.w) * vBoneWeights.w;
    } else {
        // 如果没有骨骼绑定，则使用单位矩阵
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
    // 使用模型矩阵的逆转置矩阵来正确变换法线 (处理非统一缩放)
    Normal = mat3(transpose(inverse(model))) * norm;
    
    UV = vTexCoord;
    
    // 计算光源空间位置
    FragPosLightSpace = lightSpaceMatrix * vec4(FragPos, 1.0);
    
    // 计算裁剪空间位置 (最终输出)
    gl_Position = projection * view * vec4(FragPos, 1.0);
})"},
        {"shaders/vshader_color.glsl", R"(#version 330 core
layout (location = 0) in vec3 aPos;

// 变换矩阵
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    // 计算裁剪空间中的顶点位置
    gl_Position = projection * view * model * vec4(aPos, 1.0);
})"},
        {"shaders/vshader_edge.glsl", R"(#version 330 core

// 顶点属性输入
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

// 输出到片段着色器
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

void main()
{
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

    // 描边核心逻辑：沿法线方向挤出顶点
    // 这种方法通常配合背面剔除 (Cull Front) 使用，即只绘制背面
    // 这样挤出的背面就会形成一个比原模型稍大的轮廓，看起来像描边
    vec3 pos3 = pos.xyz + norm * edge_size;
    
    FragPos = vec3(model * vec4(pos3, 1.0));
    Normal = mat3(transpose(inverse(model))) * norm;

    gl_Position = projection * view * model * vec4(pos3, 1.0);
})"},
        {"shaders/vshader_shadow.glsl", R"(#version 330 core
// 阴影生成 Pass 的顶点着色器 (带骨骼蒙皮)

layout(location = 0) in vec3 vPosition;     // 顶点位置
layout(location = 3) in ivec4 vBoneIndices; // 骨骼索引
layout(location = 4) in vec4 vBoneWeights;  // 骨骼权重

uniform mat4 lightSpaceMatrix; // 光源空间变换矩阵 (Projection * View of Light)
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

void main()
{
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
)"},
        {"shaders/vshader_shadow_static.glsl", R"(#version 330 core
// 阴影生成 Pass 的顶点着色器 (静态物体，无骨骼)

layout(location = 0) in vec3 vPosition;

uniform mat4 lightSpaceMatrix;
uniform mat4 model;

void main()
{
    // 直接变换到光源空间
    gl_Position = lightSpaceMatrix * model * vec4(vPosition, 1.0);
}
)"},
        {"shaders/vshader_stage.glsl", R"(#version 330 core
// 顶点位置
layout (location = 0) in vec3 vPosition;
// 顶点法线
layout (location = 1) in vec3 vNormal;

// 输出到片段着色器的法线
out vec3 Normal;
// 输出到片段着色器的世界空间位置
out vec3 FragPos;
// 输出到片段着色器的光空间位置（用于阴影计算）
out vec4 FragPosLightSpace;

// 模型矩阵
uniform mat4 model;
// 视图矩阵
uniform mat4 view;
// 投影矩阵
uniform mat4 projection;
// 光空间变换矩阵
uniform mat4 lightSpaceMatrix;

void main()
{
    // 计算世界空间中的顶点位置
    FragPos = vec3(model * vec4(vPosition, 1.0));
    
    // 计算变换后的法线（使用法线矩阵来处理非均匀缩放）
    Normal = mat3(transpose(inverse(model))) * vNormal;  
    
    // 将顶点位置变换到光空间
    FragPosLightSpace = lightSpaceMatrix * vec4(FragPos, 1.0);
    
    // 计算裁剪空间中的顶点位置
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
)"},
    };
    auto it = shaders.find(filename);
    if (it != shaders.end()) return it->second;
    return "";
}
