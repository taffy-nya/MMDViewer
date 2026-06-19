#include "animation/ik_solver.h"
#include "animation/skeleton.h"
#include <glm/gtc/quaternion.hpp>
#include <ranges>

void IKSolver::solve(int ik_bone_idx,
                     const std::vector<BoneDef>& defs,
                     Skeleton& skeleton) {
    const auto& ik_def = defs[ik_bone_idx];
    if (ik_def.ik_target_index == -1) return;

    const int target_idx = ik_def.ik_target_index;
    auto& states = skeleton.states();

    for (int iter = 0; iter < ik_def.ik_loop_count; ++iter) {
        bool converged = true;

        for (auto&& [idx, link] : std::views::enumerate(ik_def.ik_links)) {
            auto i = static_cast<int>(idx);
            auto& link_st = states[link.bone_index];
            const auto& link_def = defs[link.bone_index];

            glm::vec3 effector_pos = glm::vec3(states[target_idx].global_transform[3]);
            glm::vec3 target_pos   = glm::vec3(states[ik_bone_idx].global_transform[3]);
            glm::vec3 link_pos     = glm::vec3(link_st.global_transform[3]);

            glm::vec3 to_effector = glm::normalize(effector_pos - link_pos);
            glm::vec3 to_target   = glm::normalize(target_pos - link_pos);

            float dot = glm::dot(to_effector, to_target);
            if (glm::abs(dot) > 0.9999f) continue;

            float angle = glm::acos(glm::clamp(dot, -1.0f, 1.0f));
            if (glm::abs(angle) < 0.0001f) continue;

            if (ik_def.ik_limit_angle > 0.0f) {
                angle = glm::min(angle, ik_def.ik_limit_angle);
            }

            glm::vec3 axis = glm::cross(to_effector, to_target);
            if (glm::length(axis) < 0.0001f) continue;
            axis = glm::normalize(axis);

            glm::mat4 parent_global{1};
            if (link_def.parent_index != -1) {
                parent_global = states[link_def.parent_index].global_transform;
            }
            glm::mat3 inv_parent_rot = glm::transpose(glm::mat3(parent_global));
            glm::vec3 local_axis = inv_parent_rot * axis;

            glm::quat delta_rot = glm::angleAxis(angle, glm::normalize(local_axis));
            link_st.local_rotation = delta_rot * link_st.local_rotation;
            link_st.local_rotation = glm::normalize(link_st.local_rotation);

            if (link.has_limits) {
                if (glm::abs(link.min_limit.y) < 0.001f && glm::abs(link.max_limit.y) < 0.001f &&
                    glm::abs(link.min_limit.z) < 0.001f && glm::abs(link.max_limit.z) < 0.001f) {
                    float x_angle = 2.0f * std::atan2(link_st.local_rotation.x, link_st.local_rotation.w);
                    x_angle = glm::clamp(x_angle, link.min_limit.x, link.max_limit.x);
                    link_st.local_rotation = glm::angleAxis(x_angle, glm::vec3(1, 0, 0));
                }
            }

            skeleton.update_single(link.bone_index, defs);
            for (auto k : std::views::iota(0, i) | std::views::reverse) {
                skeleton.update_single(ik_def.ik_links[k].bone_index, defs);
            }
            skeleton.update_single(target_idx, defs);
            converged = false;
        }

        if (converged) break;
    }
}
