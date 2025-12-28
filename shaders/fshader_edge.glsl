#version 330 core
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
}