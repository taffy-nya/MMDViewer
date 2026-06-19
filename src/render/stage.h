#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <expected>
#include <string>
#include <memory>
#include <vector>
#include "core/model.h"
#include "core/texture.h"
#include "animation/skeleton.h"
#include "render/shader.h"

class Camera;
struct Light;

namespace render {

class ModelRenderer;

class Stage {
public:
    Stage(float size = 50.0f, int divisions = 20);
    ~Stage();

    void draw(const Camera& camera,
              const std::vector<Light>& lights,
              const glm::vec3& ambient, float ambient_strength,
              GLuint shadow_map, const glm::mat4& light_space);

    void draw_shadow(const glm::mat4& light_space);

    auto load_pmx(const std::string& path) -> std::expected<void, std::string>;
    void use_default_grid();

private:
    float size_ = 50.0f;

    // 地面平面
    Shader plane_shader_;
    GLuint plane_vao_ = 0, plane_vbo_ = 0;
    GLint plane_loc_model_, plane_loc_view_, plane_loc_proj_;
    GLint plane_loc_view_pos_, plane_loc_color_;
    GLint plane_loc_ambient_color_, plane_loc_ambient_strength_, plane_loc_num_lights_;
    GLint plane_loc_shadow_map_, plane_loc_light_space_;

    struct LightLocs {
        GLint position, direction, color, intensity, type;
        GLint constant, linear, quadratic, enabled;
    } plane_light_locs_[16];

    // 网格线
    Shader line_shader_;
    GLuint line_vao_ = 0, line_vbo_ = 0;
    GLint line_loc_model_, line_loc_view_, line_loc_proj_, line_loc_color_;
    int line_vertex_count_ = 0;

    // 阴影（默认地面）
    Shader shadow_shader_;
    GLint shadow_loc_model_, shadow_loc_light_space_;

    // PMX 舞台
    Model stage_model_;
    std::vector<TextureInfo> stage_textures_;
    Skeleton stage_skeleton_;
    std::unique_ptr<ModelRenderer> stage_renderer_;
};

} // namespace render
