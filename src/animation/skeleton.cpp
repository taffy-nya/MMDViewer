#include "animation/skeleton.h"
#include "core/model.h"
#include <glm/gtc/matrix_transform.hpp>
#include <ranges>

void Skeleton::init(const std::vector<BoneDef>& defs) {
    states_.reserve(defs.size());
    for (const auto& def : defs) {
        BoneState st;
        st.offset_matrix = glm::translate(glm::mat4(1.0f), -def.position);
        states_.push_back(st);
    }
}

void Skeleton::update_transforms(const std::vector<BoneDef>& defs) {
    for (auto i : std::views::iota(0, std::ssize(states_))) {
        update_single(static_cast<int>(i), defs);
    }
}

void Skeleton::update_single(int idx, const std::vector<BoneDef>& defs) {
    auto& st = states_[idx];
    const auto& def = defs[idx];

    glm::vec3 parent_pos{0};
    glm::mat4 parent_global{1};

    if (def.parent_index != -1) {
        parent_pos = defs[def.parent_index].position;
        parent_global = states_[def.parent_index].global_transform;
    }

    glm::vec3 relative_pos = def.position - parent_pos;

    glm::quat rotation = st.local_rotation;
    if (def.inherit_parent_index != -1 && (def.flags & 0x0100)) {
        const auto& inherit_st = states_[def.inherit_parent_index];
        glm::quat parent_rot = inherit_st.local_rotation;
        if (def.inherit_influence != 1.0f) {
            parent_rot = glm::slerp(glm::quat(1.0f, 0.0f, 0.0f, 0.0f), parent_rot, def.inherit_influence);
        }
        rotation = parent_rot * rotation;
    }

    glm::vec3 translation = st.local_translation;
    if (def.inherit_parent_index != -1 && (def.flags & 0x0200)) {
        const auto& inherit_st = states_[def.inherit_parent_index];
        translation += inherit_st.local_translation * def.inherit_influence;
    }

    glm::mat4 local = glm::translate(glm::mat4(1.0f), relative_pos + translation);
    local = local * glm::mat4_cast(rotation);
    st.global_transform = parent_global * local;
}

std::vector<glm::mat4> Skeleton::get_bone_matrices() const {
    std::vector<glm::mat4> mats;
    mats.reserve(states_.size());
    for (const auto& st : states_) {
        mats.push_back(st.global_transform * st.offset_matrix);
    }
    return mats;
}

void Skeleton::reset_pose() {
    for (auto& st : states_) {
        st.local_translation = glm::vec3(0);
        st.local_rotation = glm::quat(1, 0, 0, 0);
    }
}
