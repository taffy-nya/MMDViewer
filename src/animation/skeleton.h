#pragma once
#include "core/model.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>

struct BoneState {
    glm::vec3 local_translation{0};
    glm::quat local_rotation{1, 0, 0, 0};
    glm::mat4 global_transform{1};
    glm::mat4 offset_matrix{1};
};

class Skeleton {
public:
    Skeleton() = default;

    void init(const std::vector<BoneDef>& defs);
    void update_transforms(const std::vector<BoneDef>& defs);
    void update_single(int idx, const std::vector<BoneDef>& defs);
    std::vector<glm::mat4> get_bone_matrices() const;
    void reset_pose();

    std::vector<BoneState>& states() { return states_; }
    const std::vector<BoneState>& states() const { return states_; }
    size_t size() const { return states_.size(); }

private:
    std::vector<BoneState> states_;
};
