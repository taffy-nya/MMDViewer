#pragma once
#include "core/model.h"
#include "core/texture.h"
#include "animation/skeleton.h"
#include "animation/anim_player.h"
#include "animation/morph.h"
#include "render/model_renderer.h"
#include <expected>
#include <memory>
#include <string>
#include <vector>
#include <glm/glm.hpp>

class Camera;
struct Light;

class Model {
public:
    static auto load(const std::string& path) -> std::expected<Model, std::string>;

    Model() = default;
    ~Model() = default;
    Model(Model&&) = default;
    Model& operator=(Model&&) = default;

    auto& data()       { return data_; }
    auto& data() const { return data_; }
    auto& skeleton()       { return skeleton_; }
    auto& skeleton() const { return skeleton_; }
    auto& textures()       { return textures_; }
    auto& textures() const { return textures_; }

    auto load_motion(const std::string& path) -> std::expected<void, std::string>;
    void update_anim(float dt);
    void update_morphs();
    void update_bones();
    void reset_pose();

    void reset_transform();
    auto model_matrix() const -> glm::mat4;
    glm::vec3 translation{0};
    glm::vec3 rotation{0};
    glm::vec3 scale{1};

    void draw(const Camera& camera,
              const std::vector<Light>& lights,
              const glm::vec3& ambient, float ambient_strength,
              GLuint shadow_map, const glm::mat4& light_space);
    void draw_shadow(const glm::mat4& light_space);

    bool enable_motion{false};
    bool is_playing{true};
    bool manual_bone_control{false};
    float current_frame{0};

    AnimPlayer& anim_player() { return anim_player_; }
    MorphController& morph_ctrl() { return morph_ctrl_; }

private:
    ModelData data_;
    Skeleton skeleton_;
    std::vector<TextureInfo> textures_;
    TextureCache tex_cache_;
    std::unique_ptr<render::ModelRenderer> renderer_;
    AnimPlayer anim_player_;
    MorphController morph_ctrl_;
    std::vector<Vertex> morphed_vertices_;
    std::vector<Material> morphed_materials_;
};
