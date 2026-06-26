#pragma once
#include <expected>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <glm/glm.hpp>

#include "core/camera.h"
#include "core/light.h"
#include "render/shadow_map.h"
#include "scene/model.h"
#include "imgui.h"  // IWYU pragma: keep
#include "ImGuizmo.h"

struct GLFWwindow;

class Window;

namespace render {
class GizmoRenderer;
class SkeletonRenderer;
class Stage;
}
namespace ui {
class UiRenderer;
}

class App {
public:
    static auto create() -> std::expected<App, std::string>;
    ~App();
    App() = default;
    App(App&&) = default;
    App& operator=(App&&) = default;

    void run();

    auto load_model(const std::string& path) -> std::expected<void, std::string>;
    auto load_motion(const std::string& path) -> std::expected<void, std::string>;
    auto load_stage_pmx(const std::string& path) -> std::expected<void, std::string>;
    void reset_stage();
    void reset();

    bool has_model() const { return model_.has_value(); }
    Model& model() { return *model_; }
    const Model& model() const { return *model_; }

    void set_translation(const glm::vec3& t) { if (model_) model_->translation = t; }
    void set_rotation(const glm::vec3& r)    { if (model_) model_->rotation = r; }
    void set_scale(const glm::vec3& s)       { if (model_) model_->scale = s; }
    auto translation() const -> glm::vec3 { return model_ ? model_->translation : glm::vec3(0); }
    auto rotation() const -> glm::vec3    { return model_ ? model_->rotation : glm::vec3(0); }
    auto scale() const -> glm::vec3       { return model_ ? model_->scale : glm::vec3(1); }
    auto model_matrix() const -> glm::mat4 { return model_ ? model_->model_matrix() : glm::mat4(1.0f); }

    std::string model_path{"models/taffy/taffy.pmx"};
    std::string motion_path{"motions/TDA.vmd"};
    std::string stage_path{"Default Grid"};

    Camera camera;
    std::vector<Light> lights;
    glm::vec3 ambient_color{1};
    float ambient_strength{1};
    glm::vec4 clear_color{0.5f, 0.6f, 0.7f, 1};

    bool show_control_window{true};
    bool show_light_gizmos{true};
    bool show_skeleton{false};
    bool show_stage{true};

    bool enable_motion{false};
    bool is_playing{true};
    bool manual_bone_control{false};
    int  selected_bone_index{-1};
    int  selected_light_index{-1};

    ImGuizmo::OPERATION gizmo_op{ImGuizmo::ROTATE};
    ImGuizmo::MODE gizmo_mode{ImGuizmo::LOCAL};

    int  target_fps{60};
    bool limit_fps{true};

private:
    std::unique_ptr<Window> window_;
    float last_frame_time_{0};

    std::optional<Model> model_;

    std::unique_ptr<render::GizmoRenderer> gizmo_renderer_;
    std::unique_ptr<render::SkeletonRenderer> skeleton_renderer_;
    std::unique_ptr<render::Stage> stage_;
    render::ShadowMap shadow_map_;
    std::unique_ptr<ui::UiRenderer> ui_;

    bool mouse_left_{}, mouse_right_{}, mouse_middle_{};
    double last_x_{}, last_y_{};

    auto init() -> std::expected<void, std::string>;
    auto compute_light_space() const -> glm::mat4;
    void render_shadow_pass(const glm::mat4& light_space);
    void render_main_pass(const glm::mat4& light_space);
    void render_ui();
    void render_gizmos();

    static void on_key(GLFWwindow* w, int key, int s, int a, int m);
    static void on_mouse_btn(GLFWwindow* w, int btn, int act, int mods);
    static void on_cursor(GLFWwindow* w, double x, double y);
    static void on_scroll(GLFWwindow* w, double xoff, double yoff);
    static void on_resize(GLFWwindow* w, int width, int height);

    static auto self(GLFWwindow* w) -> App&;
};
