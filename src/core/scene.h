#pragma once
#include "model.h"
#include "anim.h"
#include "core/light.h"
#include <vector>

struct Scene {
    ModelData model;
    Animation anim;
    std::vector<Light> lights;
};
