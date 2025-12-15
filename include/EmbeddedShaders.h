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
        {"shaders/fshader_color.glsl", R"(#version 330 core
out vec4 FragColor;

uniform vec3 color;

void main()
{
    FragColor = vec4(color, 1.0);
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

in vec3 FragPos;
in vec3 Normal;

uniform vec4 edge_color;

// Light structure
struct Light {
    vec3 position;
    vec3 direction;
    vec3 color;
    float intensity;
    int type; // 0 = Directional, 1 = Point
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

void main()
{
    vec3 norm = normalize(Normal);
    vec3 lighting = vec3(0.0);
    
    // Simplified lighting for edges: just diffuse contribution
    for(int i = 0; i < numLights; i++) {
        if(!lights[i].enabled) continue;
        
        vec3 lightDir;
        float attenuation = 1.0;

        if (lights[i].type == 0) { // Directional
             lightDir = normalize(-lights[i].direction);
        } else { // Point
             lightDir = normalize(lights[i].position - FragPos);
             float distance = length(lights[i].position - FragPos);
             attenuation = 1.0 / (lights[i].constant + lights[i].linear * distance + lights[i].quadratic * (distance * distance));
        }

        float diff = max(dot(norm, lightDir), 0.0);
        lighting += diff * lights[i].color * lights[i].intensity * attenuation;
    }

    vec3 ambient = ambientStrength * ambientColor;
    vec3 result = (ambient + lighting) * edge_color.rgb;
    
    // Fallback if dark
    if (length(result) < 0.01 && length(edge_color.rgb) > 0.01) {
        // Don't let it be completely black if the edge color is not black
        // But we want it to be dark in shadow.
        // Let's just clamp it slightly so it's not invisible?
        // Actually, MMD edges DO disappear in pitch black.
    }

    fColor = vec4(result, edge_color.a);
})"},
        {"shaders/fshader_shadow.glsl", R"(#version 330 core
void main()
{             
    // gl_FragDepth is automatically written
}
)"},
        {"shaders/fshader_stage.glsl", R"(#version 330 core
out vec4 FragColor;

in vec3 Normal;
in vec3 FragPos;
in vec4 FragPosLightSpace;

struct Light {
    vec3 position;
    vec3 direction;
    vec3 color;
    float intensity;
    int type; // 0 = Directional, 1 = Point
    float constant;
    float linear;
    float quadratic;
    bool enabled;
};

#define MAX_LIGHTS 16
uniform Light lights[MAX_LIGHTS];
uniform int numLights;

uniform vec4 objectColor;
uniform vec3 viewPos;
uniform vec3 ambientColor;
uniform float ambientStrength;
uniform sampler2DShadow shadowMap;

float ShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    float currentDepth = projCoords.z;
    float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.0005);
    
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadowMap, vec3(projCoords.xy + vec2(x, y) * texelSize, currentDepth - bias)); 
            shadow += (1.0 - pcfDepth);        
        }    
    }
    shadow /= 9.0;
    
    if(projCoords.z > 1.0)
        shadow = 0.0;
        
    return shadow;
}

void main()
{
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);
    
    // Ambient
    vec3 ambient = ambientStrength * ambientColor * objectColor.rgb;
    
    vec3 lighting = vec3(0.0);
    
    for(int i = 0; i < numLights; i++) {
        if(!lights[i].enabled) continue;
        
        vec3 lightDir;
        float attenuation = 1.0;
        
        if (lights[i].type == 0) { // Directional
             lightDir = normalize(-lights[i].direction);
        } else { // Point
             lightDir = normalize(lights[i].position - FragPos);
             float distance = length(lights[i].position - FragPos);
             attenuation = 1.0 / (lights[i].constant + lights[i].linear * distance + lights[i].quadratic * (distance * distance));
        }
        
        // Diffuse
        float diff = max(dot(norm, lightDir), 0.0);
        
        float shadow = 0.0;
        if (i == 0 && lights[i].type == 0) {
             shadow = ShadowCalculation(FragPosLightSpace, norm, lightDir);
        }
        
        vec3 diffuse = diff * lights[i].color * lights[i].intensity * objectColor.rgb * (1.0 - shadow);
        
        // Specular
        vec3 halfwayDir = normalize(lightDir + viewDir);
        float spec = pow(max(dot(norm, halfwayDir), 0.0), 32.0);
        vec3 specular = 0.5 * spec * lights[i].color * lights[i].intensity * (1.0 - shadow); // White specular
        
        lighting += (diffuse + specular) * attenuation;
    }
    
    vec3 result = ambient + lighting;
    FragColor = vec4(result, objectColor.a);
}
)"},
        {"shaders/fshader_with_light.glsl", R"(#version 330 core
out vec4 fColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 UV;
in vec4 FragPosLightSpace;

struct Light {
    vec3 position;
    vec3 direction;
    vec3 color;
    float intensity;
    int type; // 0 = Directional, 1 = Point
    float constant;
    float linear;
    float quadratic;
    bool enabled;
};

#define MAX_LIGHTS 16
uniform Light lights[MAX_LIGHTS];
uniform int numLights;

uniform vec4 objectColor;   
uniform vec3 viewPos;
uniform float brightness;
uniform float shininess;
uniform vec3 specularColor;

uniform vec3 ambientColor;
uniform float ambientStrength;

uniform sampler2D textureSampler;
uniform bool hasTexture;
uniform sampler2D toonSampler;
uniform sampler2DShadow shadowMap;

float ShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    float currentDepth = projCoords.z;
    float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.0005);
    
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadowMap, vec3(projCoords.xy + vec2(x, y) * texelSize, currentDepth - bias)); 
            shadow += (1.0 - pcfDepth);        
        }    
    }
    shadow /= 9.0;
    
    if(projCoords.z > 1.0)
        shadow = 0.0;
        
    return shadow;
}

void main()
{
    vec4 textureColor = vec4(1.0);
    if (hasTexture) {
        textureColor = texture(textureSampler, UV);
    }
    
    vec3 finalObjectColor = objectColor.rgb * textureColor.rgb;
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);

    // Ambient
    vec3 ambient = ambientStrength * ambientColor * finalObjectColor;

    vec3 lighting = vec3(0.0);

    for(int i = 0; i < numLights; i++) {
        if(!lights[i].enabled) continue;
        
        vec3 lightDir;
        float attenuation = 1.0;

        if (lights[i].type == 0) { // Directional
             lightDir = normalize(-lights[i].direction);
        } else { // Point
             lightDir = normalize(lights[i].position - FragPos);
             float distance = length(lights[i].position - FragPos);
             attenuation = 1.0 / (lights[i].constant + lights[i].linear * distance + lights[i].quadratic * (distance * distance));
        }

        // Diffuse (Toon)
        float dotProd = dot(norm, lightDir);
        float lightIntensity = dotProd * 0.5 + 0.5;
        lightIntensity = clamp(lightIntensity, 0.01, 0.99); 
        vec3 toonColor = texture(toonSampler, vec2(0.5, lightIntensity)).rgb;
        
        float shadow = 0.0;
        if (i == 0 && lights[i].type == 0) { // Only first directional light casts shadow
             shadow = ShadowCalculation(FragPosLightSpace, norm, lightDir);
        }

        vec3 diffuse = toonColor * lights[i].color * lights[i].intensity * (1.0 - shadow);

        // Specular
        vec3 halfwayDir = normalize(lightDir + viewDir);
        float spec = pow(max(dot(norm, halfwayDir), 0.0), shininess);
        vec3 specular = spec * lights[i].color * lights[i].intensity * specularColor * (1.0 - shadow); 

        lighting += (diffuse * finalObjectColor + specular) * attenuation;
    }
    
    vec3 result = ambient + lighting;
    
    // Fallback if no lights or ambient is too low, just to see something
    if (numLights == 0 && ambientStrength < 0.01) {
         result = finalObjectColor;
    }

    fColor = vec4(result, textureColor.a * objectColor.a);
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
uniform mat4 lightSpaceMatrix;

uniform samplerBuffer boneMatrices;

out vec4 FragPosLightSpace;

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
    FragPosLightSpace = lightSpaceMatrix * vec4(FragPos, 1.0);
    
    gl_Position = projection * view * vec4(FragPos, 1.0);
})"},
        {"shaders/vshader_color.glsl", R"(#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0);
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

out vec3 FragPos;
out vec3 Normal;

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
    
    FragPos = vec3(model * vec4(pos3, 1.0));
    Normal = mat3(transpose(inverse(model))) * norm;

    gl_Position = projection * view * model * vec4(pos3, 1.0);
})"},
        {"shaders/vshader_shadow.glsl", R"(#version 330 core
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
)"},
        {"shaders/vshader_shadow_static.glsl", R"(#version 330 core
layout(location = 0) in vec3 vPosition;

uniform mat4 lightSpaceMatrix;
uniform mat4 model;

void main()
{
    gl_Position = lightSpaceMatrix * model * vec4(vPosition, 1.0);
}
)"},
        {"shaders/vshader_stage.glsl", R"(#version 330 core
layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;

out vec3 Normal;
out vec3 FragPos;
out vec4 FragPosLightSpace;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceMatrix;

void main()
{
    FragPos = vec3(model * vec4(vPosition, 1.0));
    Normal = mat3(transpose(inverse(model))) * vNormal;  
    FragPosLightSpace = lightSpaceMatrix * vec4(FragPos, 1.0);
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
)"},
    };
    auto it = shaders.find(filename);
    if (it != shaders.end()) return it->second;
    return "";
}
