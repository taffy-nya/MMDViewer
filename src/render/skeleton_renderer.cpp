#include "render/skeleton_renderer.h"
#include "core/camera.h"
#include "core/model.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

namespace render {

static const float k_cube_verts[] = {
    -0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f, -0.5f, -0.5f,
    -0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,  0.5f,  0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f, -0.5f,  0.5f,
    -0.5f,  0.5f,  0.5f, -0.5f,  0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,  0.5f,
     0.5f,  0.5f,  0.5f,  0.5f,  0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,  0.5f,
    -0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f, -0.5f,
    -0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,  0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f, -0.5f
};

static const float k_line_verts[] = {
    0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f
};

auto SkeletonRenderer::create() -> std::expected<SkeletonRenderer, std::string> {
    SkeletonRenderer r;

    auto shader_result = Shader::from_paths("shaders/vshader_color.glsl", "shaders/fshader_color.glsl");
    if (!shader_result) return std::unexpected(shader_result.error());
    r.shader_ = std::move(*shader_result);

    r.loc_model_ = r.shader_.uniform("model");
    r.loc_view_  = r.shader_.uniform("view");
    r.loc_proj_  = r.shader_.uniform("projection");
    r.loc_color_ = r.shader_.uniform("color");

    glGenVertexArrays(1, &r.cube_vao_);
    glGenBuffers(1, &r.cube_vbo_);
    glBindVertexArray(r.cube_vao_);
    glBindBuffer(GL_ARRAY_BUFFER, r.cube_vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(k_cube_verts), k_cube_verts, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);

    glGenVertexArrays(1, &r.line_vao_);
    glGenBuffers(1, &r.line_vbo_);
    glBindVertexArray(r.line_vao_);
    glBindBuffer(GL_ARRAY_BUFFER, r.line_vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(k_line_verts), k_line_verts, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
    return r;
}

SkeletonRenderer::~SkeletonRenderer() {
    if (cube_vao_ != 0) glDeleteVertexArrays(1, &cube_vao_);
    if (cube_vbo_ != 0) glDeleteBuffers(1, &cube_vbo_);
    if (line_vao_ != 0) glDeleteVertexArrays(1, &line_vao_);
    if (line_vbo_ != 0) glDeleteBuffers(1, &line_vbo_);
}

SkeletonRenderer::SkeletonRenderer(SkeletonRenderer&& other) noexcept
    : shader_(std::move(other.shader_))
    , cube_vao_(other.cube_vao_), cube_vbo_(other.cube_vbo_)
    , line_vao_(other.line_vao_), line_vbo_(other.line_vbo_)
    , loc_model_(other.loc_model_), loc_view_(other.loc_view_)
    , loc_proj_(other.loc_proj_), loc_color_(other.loc_color_) {
    other.cube_vao_ = 0; other.cube_vbo_ = 0;
    other.line_vao_ = 0; other.line_vbo_ = 0;
}

SkeletonRenderer& SkeletonRenderer::operator=(SkeletonRenderer&& other) noexcept {
    if (this != &other) {
        if (cube_vao_ != 0) glDeleteVertexArrays(1, &cube_vao_);
        if (cube_vbo_ != 0) glDeleteBuffers(1, &cube_vbo_);
        if (line_vao_ != 0) glDeleteVertexArrays(1, &line_vao_);
        if (line_vbo_ != 0) glDeleteBuffers(1, &line_vbo_);
        shader_ = std::move(other.shader_);
        cube_vao_ = other.cube_vao_; cube_vbo_ = other.cube_vbo_;
        line_vao_ = other.line_vao_; line_vbo_ = other.line_vbo_;
        loc_model_ = other.loc_model_; loc_view_ = other.loc_view_;
        loc_proj_ = other.loc_proj_; loc_color_ = other.loc_color_;
        other.cube_vao_ = 0; other.cube_vbo_ = 0;
        other.line_vao_ = 0; other.line_vbo_ = 0;
    }
    return *this;
}

void SkeletonRenderer::draw(const Camera& camera,
                             const glm::mat4& model_matrix,
                             const std::vector<BoneDef>& bone_defs,
                             const std::vector<glm::mat4>& global_transforms,
                             int selected_bone) {
    if (!shader_) return;

    GLboolean depth_enabled = glIsEnabled(GL_DEPTH_TEST);
    glDisable(GL_DEPTH_TEST);

    shader_.use();
    shader_.set_mat4("view", camera.get_view_matrix());
    shader_.set_mat4("projection", camera.get_projection_matrix());

    // 绘制骨骼连线
    glBindVertexArray(line_vao_);

    int bone_count = static_cast<int>(global_transforms.size());
    for (int i = 0; i < bone_count; ++i) {
        if (i >= static_cast<int>(bone_defs.size())) break;
        const auto& def = bone_defs[i];
        if (def.parent_index < 0 || def.parent_index >= bone_count) continue;

        glm::vec3 start = glm::vec3(global_transforms[def.parent_index][3]);
        glm::vec3 end   = glm::vec3(global_transforms[i][3]);

        start = glm::vec3(model_matrix * glm::vec4(start, 1.0f));
        end   = glm::vec3(model_matrix * glm::vec4(end, 1.0f));

        glm::vec3 dir = end - start;
        float len = glm::length(dir);
        if (len < 0.001f) continue;

        glm::vec3 direction = glm::normalize(dir);
        glm::quat rot = glm::rotation(glm::vec3(0.0f, 0.0f, 1.0f), direction);
        glm::mat4 model = glm::translate(glm::mat4(1.0f), start)
                        * glm::toMat4(rot)
                        * glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, 1.0f, len));
        shader_.set_mat4("model", model);

        if (i == selected_bone) {
            shader_.set_vec3("color", glm::vec3(1.0f, 1.0f, 0.0f));
        } else {
            shader_.set_vec3("color", glm::vec3(0.0f, 1.0f, 0.0f));
        }

        glDrawArrays(GL_LINES, 0, 2);
    }

    // 绘制关节立方体
    glBindVertexArray(cube_vao_);

    for (int i = 0; i < bone_count; ++i) {
        glm::vec3 pos = glm::vec3(global_transforms[i][3]);
        pos = glm::vec3(model_matrix * glm::vec4(pos, 1.0f));

        glm::mat4 model = glm::translate(glm::mat4(1.0f), pos)
                        * glm::scale(glm::mat4(1.0f), glm::vec3(0.1f));
        shader_.set_mat4("model", model);

        if (i == selected_bone) {
            shader_.set_vec3("color", glm::vec3(1.0f, 0.0f, 0.0f));
        } else {
            shader_.set_vec3("color", glm::vec3(0.0f, 0.0f, 1.0f));
        }

        glDrawArrays(GL_TRIANGLES, 0, 36);
    }

    glBindVertexArray(0);

    if (depth_enabled) glEnable(GL_DEPTH_TEST);
}

} // namespace render
