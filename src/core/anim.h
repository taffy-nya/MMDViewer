#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>
#include <array>
#include <string>

struct BoneKeyframe {
    int frame{0};
    glm::vec3 translation{0};
    glm::quat rotation{1, 0, 0, 0};
    std::array<uint8_t, 64> bezier_params{};
};

struct BoneTrack {
    std::string bone_name;
    std::vector<BoneKeyframe> keyframes;
};

struct MorphKeyframe {
    int frame{0};
    float weight{0.0f};
};

struct MorphTrack {
    std::string morph_name;
    std::vector<MorphKeyframe> keyframes;
};

struct Animation {
    std::vector<BoneTrack> tracks;
    std::vector<MorphTrack> morph_tracks;
};
