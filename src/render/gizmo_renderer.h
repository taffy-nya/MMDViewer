#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <expected>
#include <string>
#include "render/shader.h"

class Camera;
struct Light;

namespace render {

class GizmoRenderer {
public:
    static auto create() -> std::expected<GizmoRenderer, std::string>;

    GizmoRenderer() = default;
    ~GizmoRenderer();
    GizmoRenderer(GizmoRenderer&& other) noexcept;
    GizmoRenderer& operator=(GizmoRenderer&& other) noexcept;
    GizmoRenderer(const GizmoRenderer&) = delete;
    GizmoRenderer& operator=(const GizmoRenderer&) = delete;

    void draw_lights(const Camera& camera, const std::vector<Light>& lights);

private:
    Shader shader_;
    GLuint cube_vao_ = 0, cube_vbo_ = 0;
    GLuint line_vao_ = 0, line_vbo_ = 0;

    GLint loc_model_, loc_view_, loc_proj_, loc_color_;
};

} // namespace render
