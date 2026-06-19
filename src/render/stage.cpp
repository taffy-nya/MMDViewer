#include "render/stage.h"
#include "render/model_renderer.h"
#include "core/model_loader.h"
#include "core/camera.h"
#include "core/light.h"
#include <print>
#include <glm/gtc/type_ptr.hpp>

namespace render {

Stage::Stage(float size, int divisions) : size_(size) {
    auto p_result = Shader::from_paths("shaders/vshader_stage.glsl", "shaders/fshader_stage.glsl");
    if (p_result) {
        plane_shader_ = std::move(*p_result);
    } else {
        std::println(stderr, "stage plane shader error: {}", p_result.error());
    }

    auto sp_result = Shader::from_paths("shaders/vshader_shadow_static.glsl", "shaders/fshader_shadow.glsl");
    if (sp_result) {
        shadow_shader_ = std::move(*sp_result);
    } else {
        std::println(stderr, "stage shadow shader error: {}", sp_result.error());
    }

    auto pl_result = Shader::from_paths("shaders/vshader_color.glsl", "shaders/fshader_color.glsl");
    if (pl_result) {
        line_shader_ = std::move(*pl_result);
    } else {
        std::println(stderr, "stage lines shader error: {}", pl_result.error());
    }

    plane_loc_model_      = plane_shader_.uniform("model");
    plane_loc_view_       = plane_shader_.uniform("view");
    plane_loc_proj_       = plane_shader_.uniform("projection");
    plane_loc_color_      = plane_shader_.uniform("objectColor");
    plane_loc_view_pos_   = plane_shader_.uniform("viewPos");
    plane_loc_ambient_color_    = plane_shader_.uniform("ambientColor");
    plane_loc_ambient_strength_ = plane_shader_.uniform("ambientStrength");
    plane_loc_num_lights_       = plane_shader_.uniform("numLights");
    plane_loc_shadow_map_       = plane_shader_.uniform("shadowMap");
    plane_loc_light_space_      = plane_shader_.uniform("lightSpaceMatrix");

    for (int i = 0; i < 16; ++i) {
        std::string base = "lights[" + std::to_string(i) + "]";
        plane_light_locs_[i].position  = plane_shader_.uniform((base + ".position").c_str());
        plane_light_locs_[i].direction = plane_shader_.uniform((base + ".direction").c_str());
        plane_light_locs_[i].color     = plane_shader_.uniform((base + ".color").c_str());
        plane_light_locs_[i].intensity = plane_shader_.uniform((base + ".intensity").c_str());
        plane_light_locs_[i].type      = plane_shader_.uniform((base + ".type").c_str());
        plane_light_locs_[i].constant  = plane_shader_.uniform((base + ".constant").c_str());
        plane_light_locs_[i].linear    = plane_shader_.uniform((base + ".linear").c_str());
        plane_light_locs_[i].quadratic = plane_shader_.uniform((base + ".quadratic").c_str());
        plane_light_locs_[i].enabled   = plane_shader_.uniform((base + ".enabled").c_str());
    }

    shadow_loc_model_          = shadow_shader_.uniform("model");
    shadow_loc_light_space_    = shadow_shader_.uniform("lightSpaceMatrix");

    line_loc_model_  = line_shader_.uniform("model");
    line_loc_view_   = line_shader_.uniform("view");
    line_loc_proj_   = line_shader_.uniform("projection");
    line_loc_color_  = line_shader_.uniform("color");

    std::vector<glm::vec3> line_verts;
    float step = size * 2 / divisions;
    for (int i = 0; i <= divisions; ++i) {
        float pos = -size + i * step;
        line_verts.push_back(glm::vec3(-size, 0.01f, pos));
        line_verts.push_back(glm::vec3(size, 0.01f, pos));
        line_verts.push_back(glm::vec3(pos, 0.01f, -size));
        line_verts.push_back(glm::vec3(pos, 0.01f, size));
    }
    line_vertex_count_ = static_cast<int>(line_verts.size());

    glGenVertexArrays(1, &line_vao_);
    glBindVertexArray(line_vao_);
    glGenBuffers(1, &line_vbo_);
    glBindBuffer(GL_ARRAY_BUFFER, line_vbo_);
    glBufferData(GL_ARRAY_BUFFER, line_verts.size() * sizeof(glm::vec3), line_verts.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(0);

    float plane_data[] = {
        -size, 0.0f, -size, 0.0f, 1.0f, 0.0f,
        -size, 0.0f,  size, 0.0f, 1.0f, 0.0f,
         size, 0.0f,  size, 0.0f, 1.0f, 0.0f,
        -size, 0.0f, -size, 0.0f, 1.0f, 0.0f,
         size, 0.0f,  size, 0.0f, 1.0f, 0.0f,
         size, 0.0f, -size, 0.0f, 1.0f, 0.0f
    };

    glGenVertexArrays(1, &plane_vao_);
    glBindVertexArray(plane_vao_);
    glGenBuffers(1, &plane_vbo_);
    glBindBuffer(GL_ARRAY_BUFFER, plane_vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(plane_data), plane_data, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                          reinterpret_cast<const void*>(static_cast<uintptr_t>(3 * sizeof(float))));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

Stage::~Stage() {
    if (plane_vao_ != 0) glDeleteVertexArrays(1, &plane_vao_);
    if (plane_vbo_ != 0) glDeleteBuffers(1, &plane_vbo_);
    if (line_vao_ != 0) glDeleteVertexArrays(1, &line_vao_);
    if (line_vbo_ != 0) glDeleteBuffers(1, &line_vbo_);
}

auto Stage::load_pmx(const std::string& path) -> std::expected<void, std::string> {
    use_default_grid();

    auto model_result = core::load_pmx(path);
    if (!model_result) return std::unexpected(model_result.error());
    stage_model_ = std::move(*model_result);

    auto last_slash = path.rfind('/');
    if (last_slash == std::string::npos) last_slash = path.rfind('\\');
    auto base_path = (last_slash != std::string::npos) ? path.substr(0, last_slash + 1) : "";

    TextureCache tex_cache;
    stage_textures_.clear();
    for (const auto& tex_path : stage_model_.tex_paths) {
        TextureInfo info;
        info.path = tex_path;
        if (!tex_path.empty()) {
            std::string full_path = base_path + tex_path;
            for (char& c : full_path) if (c == '\\') c = '/';
            auto tex_result = tex_cache.get_or_load(full_path);
            if (tex_result) info.gl_texture_id = (*tex_result)->id();
        }
        stage_textures_.push_back(info);
    }

    stage_skeleton_.init(stage_model_.bone_defs);
    stage_skeleton_.update_transforms(stage_model_.bone_defs);

    auto renderer = ModelRenderer::create(stage_model_, stage_textures_);
    if (!renderer) return std::unexpected(renderer.error());
    stage_renderer_ = std::make_unique<ModelRenderer>(std::move(*renderer));
    return {};
}

void Stage::use_default_grid() {
    stage_renderer_.reset();
    stage_textures_.clear();
    stage_model_ = Model{};
    stage_skeleton_ = Skeleton{};
}

void Stage::draw_shadow(const glm::mat4& light_space) {
    if (stage_renderer_) {
        glm::mat4 ident(1.0f);
        auto bone_mats = stage_skeleton_.get_bone_matrices();
        stage_renderer_->draw_shadow(ident, bone_mats, light_space);
        return;
    }

    if (!shadow_shader_) return;

    shadow_shader_.use();
    shadow_shader_.set_mat4("model", glm::mat4(1.0f));
    shadow_shader_.set_mat4("lightSpaceMatrix", light_space);

    glBindVertexArray(plane_vao_);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void Stage::draw(const Camera& camera,
                 const std::vector<Light>& lights,
                 const glm::vec3& ambient, float ambient_strength,
                 GLuint shadow_map, const glm::mat4& light_space) {
    if (stage_renderer_) {
        glm::mat4 ident(1.0f);
        auto bone_mats = stage_skeleton_.get_bone_matrices();
        stage_renderer_->draw(camera, ident, bone_mats, lights, ambient, ambient_strength,
                              shadow_map, light_space);
        return;
    }

    // 默认网格
    glm::mat4 model(1.0f);
    glm::mat4 view = camera.get_view_matrix();
    glm::mat4 proj = camera.get_projection_matrix();
    glm::vec3 view_pos = camera.position;

    plane_shader_.use();
    plane_shader_.set_mat4("model", model);
    plane_shader_.set_mat4("view", view);
    plane_shader_.set_mat4("projection", proj);
    plane_shader_.set_vec3("viewPos", view_pos);
    plane_shader_.set_vec4("objectColor", glm::vec4(1.0f));
    plane_shader_.set_mat4("lightSpaceMatrix", light_space);

    glActiveTexture(GL_TEXTURE10);
    glBindTexture(GL_TEXTURE_2D, shadow_map);
    plane_shader_.set_int("shadowMap", 10);

    plane_shader_.set_vec3("ambientColor", ambient);
    plane_shader_.set_float("ambientStrength", ambient_strength);
    plane_shader_.set_int("numLights", static_cast<int>(lights.size()));

    for (int i = 0; i < static_cast<int>(lights.size()) && i < 16; ++i) {
        const auto& light = lights[i];
        const auto& locs = plane_light_locs_[i];
        glUniform3fv(locs.position,  1, glm::value_ptr(light.position));
        glUniform3fv(locs.direction, 1, glm::value_ptr(light.direction));
        glUniform3fv(locs.color,     1, glm::value_ptr(light.color));
        glUniform1f(locs.intensity, light.intensity);
        glUniform1i(locs.type,     light.type);
        glUniform1f(locs.constant,  light.constant);
        glUniform1f(locs.linear,    light.linear);
        glUniform1f(locs.quadratic, light.quadratic);
        glUniform1i(locs.enabled,  light.enabled);
    }

    glBindVertexArray(plane_vao_);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    line_shader_.use();
    line_shader_.set_mat4("model", model);
    line_shader_.set_mat4("view", view);
    line_shader_.set_mat4("projection", proj);

    glBindVertexArray(line_vao_);
    line_shader_.set_vec3("color", glm::vec3(0.5f, 0.5f, 0.5f));
    glDrawArrays(GL_LINES, 0, line_vertex_count_);

    glBindVertexArray(0);
    glUseProgram(0);
}

} // namespace render
