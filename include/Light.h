#pragma once
#include "glm/glm.hpp"

enum LightType {
    LIGHT_DIRECTIONAL = 0, // 平行光
    LIGHT_POINT = 1        // 点光源
};

struct Light {
    glm::vec3 position;  // 光源位置 (仅用于点光源)
    glm::vec3 direction; // 光照方向 (仅用于平行光)
    glm::vec3 color;     // 颜色
    float intensity;     // 强度
    int type;            // 类型: 0 = 平行光, 1 = 点光源
    
    // 点光源的衰减
    // (1.0 / (constant + linear * dist + quadratic * dist ^ 2)
    float constant;
    float linear;
    float quadratic;

    bool enabled;

    Light() : 
        position(0.0f), direction(0.0f, -1.0f, 0.0f), color(1.0f), intensity(0.5f), type(LIGHT_POINT),
        constant(1.0f), linear(0.09f), quadratic(0.032f), enabled(true) {}
};
