#include "render/gizmo_renderer.h"
#include "core/camera.h"
#include "core/light.h"
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

auto GizmoRenderer::create() -> std::expected<GizmoRenderer, std::string> {
    GizmoRenderer r;

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

GizmoRenderer::~GizmoRenderer() {
    if (cube_vao_ != 0) glDeleteVertexArrays(1, &cube_vao_);
    if (cube_vbo_ != 0) glDeleteBuffers(1, &cube_vbo_);
    if (line_vao_ != 0) glDeleteVertexArrays(1, &line_vao_);
    if (line_vbo_ != 0) glDeleteBuffers(1, &line_vbo_);
}

GizmoRenderer::GizmoRenderer(GizmoRenderer&& other) noexcept
    : shader_(std::move(other.shader_))
    , cube_vao_(other.cube_vao_), cube_vbo_(other.cube_vbo_)
    , line_vao_(other.line_vao_), line_vbo_(other.line_vbo_)
    , loc_model_(other.loc_model_), loc_view_(other.loc_view_)
    , loc_proj_(other.loc_proj_), loc_color_(other.loc_color_) {
    other.cube_vao_ = 0; other.cube_vbo_ = 0;
    other.line_vao_ = 0; other.line_vbo_ = 0;
}

GizmoRenderer& GizmoRenderer::operator=(GizmoRenderer&& other) noexcept {
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

void GizmoRenderer::draw_lights(const Camera& camera, const std::vector<Light>& lights) {
    if (!shader_) return;

    shader_.use();
    shader_.set_mat4("view", camera.get_view_matrix());
    shader_.set_mat4("projection", camera.get_projection_matrix());

    glBindVertexArray(cube_vao_);

    for (const auto& light : lights) {
        if (!light.enabled) continue;

        if (light.type == LIGHT_POINT) {
            glm::mat4 model = glm::translate(glm::mat4(1.0f), light.position)
                            * glm::scale(glm::mat4(1.0f), glm::vec3(0.5f));
            shader_.set_mat4("model", model);
            shader_.set_vec3("color", light.color);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        } else {
            glm::vec3 dir = glm::normalize(light.direction);
            glm::vec3 source = -dir * 15.0f;

            glm::mat4 model = glm::translate(glm::mat4(1.0f), source)
                            * glm::scale(glm::mat4(1.0f), glm::vec3(0.5f));
            shader_.set_mat4("model", model);
            shader_.set_vec3("color", light.color);
            glBindVertexArray(cube_vao_);
            glDrawArrays(GL_TRIANGLES, 0, 36);

            glm::quat rot = glm::rotation(glm::vec3(0.0f, 0.0f, 1.0f), dir);
            model = glm::translate(glm::mat4(1.0f), source)
                  * glm::toMat4(rot)
                  * glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, 1.0f, 5.0f));
            shader_.set_mat4("model", model);
            glBindVertexArray(line_vao_);
            glDrawArrays(GL_LINES, 0, 2);
        }
    }

    glBindVertexArray(0);
}

} // namespace render
