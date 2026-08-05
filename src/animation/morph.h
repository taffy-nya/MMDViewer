#pragma once
#include "core/model.h"
#include <string>
#include <unordered_map>
#include <vector>

struct BoneState;

class MorphController {
public:
    void init(const ModelData& data);
    void reset();

    void set_target_weight(int idx, float w);
    float target_weight(int idx) const;
    float final_weight(int idx) const;

    void resolve(const ModelData& data);
    bool is_vertex_uv_dirty() const { return vertex_uv_dirty_; }

    void apply_bone_morphs(const ModelData& data, std::vector<BoneState>& states);
    void apply_vertex_morphs(const ModelData& data, std::vector<Vertex>& vertices);
    void apply_uv_morphs(const ModelData& data, std::vector<Vertex>& vertices);
    void apply_material(const ModelData& data, int mat_idx, Material& out) const;

    int find_morph(const std::string& name) const;
    int morph_count() const { return static_cast<int>(target_weights_.size()); }
    const std::vector<float>& target_weights() const { return target_weights_; }
    const std::vector<float>& final_weights() const { return final_weights_; }

private:
    std::vector<std::string> morph_names_;
    std::vector<MorphType> morph_types_;
    std::unordered_map<std::string, int> name_to_idx_;
    std::vector<float> target_weights_;
    std::vector<float> final_weights_;
    std::vector<float> last_final_weights_;
    bool vertex_uv_dirty_{true};

    void resolve_from(const ModelData& data, int idx, float weight,
                      std::vector<float>& out, std::vector<bool>& visiting);

    static bool is_leaf_type(MorphType t);
};
