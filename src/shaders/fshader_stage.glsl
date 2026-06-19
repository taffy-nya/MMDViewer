#version 330 core
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
