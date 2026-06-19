#pragma once
#include "model.h"
#include "anim.h"
#include "core/light.h"
#include <vector>

struct Scene {
    Model model;
    Animation anim;
    std::vector<Light> lights;
};
