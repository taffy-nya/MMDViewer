#include "animation/morph.h"
#include "animation/skeleton.h"
#include <cmath>
#include <glm/gtc/quaternion.hpp>

void MorphController::init(const ModelData& data) {
    const auto& defs = data.morph_defs;
    auto n = static_cast<int>(defs.size());
    morph_names_.reserve(n);
    morph_types_.reserve(n);
    name_to_idx_.clear();
    for (int i = 0; i < n; ++i) {
        morph_names_.push_back(defs[i].name);
        morph_types_.push_back(defs[i].type);
        name_to_idx_[defs[i].name] = i;
    }
    target_weights_.assign(n, 0.0f);
    final_weights_.assign(n, 0.0f);
    last_final_weights_.assign(n, 0.0f);
    vertex_uv_dirty_ = true;
}

void MorphController::reset() {
    std::fill(target_weights_.begin(), target_weights_.end(), 0.0f);
    vertex_uv_dirty_ = true;
}

void MorphController::set_target_weight(int idx, float w) {
    if (idx >= 0 && idx < static_cast<int>(target_weights_.size())) {
        target_weights_[idx] = w;
    }
}

float MorphController::target_weight(int idx) const {
    if (idx >= 0 && idx < static_cast<int>(target_weights_.size())) {
        return target_weights_[idx];
    }
    return 0.0f;
}

float MorphController::final_weight(int idx) const {
    if (idx >= 0 && idx < static_cast<int>(final_weights_.size())) {
        return final_weights_[idx];
    }
    return 0.0f;
}

bool MorphController::is_leaf_type(MorphType t) {
    return t == MorphType::Vertex || t == MorphType::Bone ||
           t == MorphType::UV || t == MorphType::Material ||
           t == MorphType::AdditionalUV1 || t == MorphType::AdditionalUV2 ||
           t == MorphType::AdditionalUV3 || t == MorphType::AdditionalUV4;
}

void MorphController::resolve_from(const ModelData& data, int idx, float weight,
                                    std::vector<float>& out, std::vector<bool>& visiting) {
    if (visiting[idx]) return;
    visiting[idx] = true;

    const auto& def = data.morph_defs[idx];
    if (is_leaf_type(def.type)) {
        out[idx] += weight;
    } else if (def.type == MorphType::Group) {
        for (const auto& off : def.group_offsets) {
            if (off.morph_index >= 0 && off.morph_index < static_cast<int>(data.morph_defs.size())) {
                resolve_from(data, off.morph_index, weight * off.weight_ratio, out, visiting);
            }
        }
    }
    visiting[idx] = false;
}

void MorphController::resolve(const ModelData& data) {
    const auto& defs = data.morph_defs;
    size_t n = defs.size();
    if (n == 0) return;

    final_weights_.assign(n, 0.0f);

    std::vector<bool> visiting(n, false);
    for (size_t i = 0; i < n; ++i) {
        if (target_weights_[i] != 0.0f) {
            resolve_from(data, static_cast<int>(i), target_weights_[i], final_weights_, visiting);
        }
    }

    vertex_uv_dirty_ = false;
    for (size_t i = 0; i < n; ++i) {
        if (morph_types_[i] == MorphType::Vertex || morph_types_[i] == MorphType::UV) {
            if (std::abs(final_weights_[i] - last_final_weights_[i]) > 1e-6f) {
                vertex_uv_dirty_ = true;
                break;
            }
        }
    }
    last_final_weights_ = final_weights_;
}

void MorphController::apply_bone_morphs(const ModelData& data, std::vector<BoneState>& states) {
    const auto& defs = data.morph_defs;
    for (size_t i = 0; i < defs.size(); ++i) {
        if (defs[i].type != MorphType::Bone) continue;
        float w = final_weights_[i];
        if (w == 0.0f) continue;

        for (const auto& off : defs[i].bone_offsets) {
            if (off.bone_index < 0 || off.bone_index >= static_cast<int>(states.size())) continue;
            auto& st = states[off.bone_index];
            st.local_translation += off.translation_delta * w;

            glm::quat target_q(off.rotation_quat.w, off.rotation_quat.x,
                              off.rotation_quat.y, off.rotation_quat.z);
            glm::quat identity(1, 0, 0, 0);
            glm::quat rot = glm::slerp(identity, target_q, w);
            st.local_rotation = st.local_rotation * rot;
        }
    }
}

void MorphController::apply_vertex_morphs(const ModelData& data, std::vector<Vertex>& vertices) {
    const auto& defs = data.morph_defs;
    for (size_t i = 0; i < defs.size(); ++i) {
        if (defs[i].type != MorphType::Vertex) continue;
        float w = final_weights_[i];
        if (w == 0.0f) continue;

        for (const auto& off : defs[i].vertex_offsets) {
            if (off.vertex_index >= 0 && off.vertex_index < static_cast<int>(vertices.size())) {
                vertices[off.vertex_index].position += off.position_delta * w;
            }
        }
    }
}

void MorphController::apply_uv_morphs(const ModelData& data, std::vector<Vertex>& vertices) {
    const auto& defs = data.morph_defs;
    for (size_t i = 0; i < defs.size(); ++i) {
        if (defs[i].type != MorphType::UV) continue;
        float w = final_weights_[i];
        if (w == 0.0f) continue;

        for (const auto& off : defs[i].uv_offsets) {
            if (off.vertex_index >= 0 && off.vertex_index < static_cast<int>(vertices.size())) {
                vertices[off.vertex_index].uv += glm::vec2(off.uv_delta) * w;
            }
        }
    }
}

static void apply_morph_multiply(float& val, float morph_val, float weight) {
    float mul = 1.0f + (morph_val - 1.0f) * weight;
    val *= mul;
}

static void apply_morph_multiply(glm::vec3& val, const glm::vec3& morph_val, float weight) {
    for (int k = 0; k < 3; ++k) {
        float mul = 1.0f + (morph_val[k] - 1.0f) * weight;
        val[k] *= mul;
    }
}

static void apply_morph_multiply(glm::vec4& val, const glm::vec4& morph_val, float weight) {
    for (int k = 0; k < 4; ++k) {
        float mul = 1.0f + (morph_val[k] - 1.0f) * weight;
        val[k] *= mul;
    }
}

static void apply_morph_add(float& val, float morph_val, float weight) {
    val += morph_val * weight;
}

static void apply_morph_add(glm::vec3& val, const glm::vec3& morph_val, float weight) {
    val += morph_val * weight;
}

static void apply_morph_add(glm::vec4& val, const glm::vec4& morph_val, float weight) {
    val += morph_val * weight;
}

void MorphController::apply_material(const ModelData& data, int mat_idx, Material& out) const {
    const auto& defs = data.morph_defs;
    for (size_t i = 0; i < defs.size(); ++i) {
        if (defs[i].type != MorphType::Material) continue;
        float w = final_weights_[i];
        if (w == 0.0f) continue;

        for (const auto& off : defs[i].material_offsets) {
            if (off.material_index != -1 && off.material_index != mat_idx) continue;

            if (off.calc_mode == 0) {
                apply_morph_multiply(out.diffuse, off.diffuse, w);
                apply_morph_multiply(out.specular, off.specular, w);
                apply_morph_multiply(out.shininess, off.shininess, w);
                apply_morph_multiply(out.ambient, off.ambient, w);
                apply_morph_multiply(out.edge_color, off.edge_color, w);
                apply_morph_add(out.edge_size, off.edge_size, w);
            } else {
                apply_morph_add(out.diffuse, off.diffuse, w);
                apply_morph_add(out.specular, off.specular, w);
                apply_morph_add(out.shininess, off.shininess, w);
                apply_morph_add(out.ambient, off.ambient, w);
                apply_morph_add(out.edge_color, off.edge_color, w);
                apply_morph_add(out.edge_size, off.edge_size, w);
            }
        }
    }
}

int MorphController::find_morph(const std::string& name) const {
    auto it = name_to_idx_.find(name);
    if (it != name_to_idx_.end()) return it->second;
    return -1;
}
