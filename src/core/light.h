#pragma once
#include <glm/glm.hpp>

enum LightType {
    LIGHT_DIRECTIONAL = 0,
    LIGHT_POINT = 1
};

struct Light {
    glm::vec3 position{0};
    glm::vec3 direction{0, -1, 0};
    glm::vec3 color{1};
    float intensity{0.5f};
    int type{LIGHT_POINT};

    float constant{1};
    float linear{0.09f};
    float quadratic{0.032f};

    bool enabled{true};
};
