#pragma once
#include "glm/glm.hpp"

enum LightType {
    LIGHT_DIRECTIONAL = 0,
    LIGHT_POINT = 1
};

struct Light {
    glm::vec3 position;  // For point lights
    glm::vec3 direction; // For directional lights
    glm::vec3 color;
    float intensity;
    int type; // 0 = Directional, 1 = Point
    
    // Attenuation for point lights
    float constant;
    float linear;
    float quadratic;

    bool enabled;

    Light() : 
        position(0.0f), direction(0.0f, -1.0f, 0.0f), color(1.0f), intensity(0.5f), type(LIGHT_POINT),
        constant(1.0f), linear(0.09f), quadratic(0.032f), enabled(true) {}
};
