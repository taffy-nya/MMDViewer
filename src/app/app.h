#pragma once
#include <expected>
#include <memory>
#include <string>
#include <vector>
#include <glm/glm.hpp>

#include "core/camera.h"
#include "core/light.h"
#include "core/model.h"
#include "core/texture.h"
#include "animation/skeleton.h"
#include "animation/anim_player.h"
#include "render/shadow_map.h"
#include "imgui.h"
#include "ImGuizmo.h"

struct GLFWwindow;

class Window;

namespace render {
class ModelRenderer;
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

    // 模型数据访问（UI 面板用）
    auto& model() { return model_; }
    auto& skeleton() { return skeleton_; }
    auto& textures() { return textures_; }

    // 模型变换
    void set_translation(const glm::vec3& t) { model_trans_ = t; }
    void set_rotation(const glm::vec3& r) { model_rot_ = r; }
    void set_scale(const glm::vec3& s) { model_scale_ = s; }
    auto translation() const -> glm::vec3 { return model_trans_; }
    auto rotation() const -> glm::vec3 { return model_rot_; }
    auto scale() const -> glm::vec3 { return model_scale_; }
    auto model_matrix() const -> glm::mat4;

    // ── 公开状态（UI 面板直接读写）──
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

    Model model_;
    Skeleton skeleton_;
    std::vector<TextureInfo> textures_;
    TextureCache tex_cache_;
    glm::vec3 model_trans_{0}, model_rot_{0}, model_scale_{1};

    std::unique_ptr<render::ModelRenderer> model_renderer_;
    std::unique_ptr<render::GizmoRenderer> gizmo_renderer_;
    std::unique_ptr<render::SkeletonRenderer> skeleton_renderer_;
    std::unique_ptr<render::Stage> stage_;
    render::ShadowMap shadow_map_;
    AnimPlayer anim_player_;
    std::unique_ptr<ui::UiRenderer> ui_;

    float current_frame_{0};

    bool mouse_left_{}, mouse_right_{}, mouse_middle_{};
    double last_x_{}, last_y_{};

    // ── 内部方法 ──
    auto init() -> std::expected<void, std::string>;
    auto compute_light_space() const -> glm::mat4;
    void update_anim(float dt);
    void update_bones();
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
