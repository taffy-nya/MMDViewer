#include "app/app.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <print>

#include "render/gizmo_renderer.h"
#include "render/skeleton_renderer.h"
#include "render/stage.h"
#include "platform/window.h"
#include "platform/timer.h"
#include "ui/ui_renderer.h"

#include "ImGuizmo.h"


void render_general_panel(App& app);
void render_model_panel(App& app, GLFWwindow* win);
void render_skeleton_panel(App& app);
void render_stage_panel(App& app, GLFWwindow* win);
void render_lighting_panel(App& app);


App::~App() = default;

auto App::create() -> std::expected<App, std::string> {
    auto win = Window::create({.width = 1920, .height = 1080, .title = "MMDViewer", .vsync = true});
    if (!win) return std::unexpected(win.error());

    App app;
    app.window_ = std::make_unique<Window>(std::move(*win));

    auto* handle = app.window_->get_handle();
    glfwSetWindowUserPointer(handle, &app);
    glfwSetFramebufferSizeCallback(handle, on_resize);
    glfwSetKeyCallback(handle, on_key);
    glfwSetMouseButtonCallback(handle, on_mouse_btn);
    glfwSetCursorPosCallback(handle, on_cursor);
    glfwSetScrollCallback(handle, on_scroll);

    auto init_result = app.init();
    if (!init_result) {
        return std::unexpected(init_result.error());
    }
    return app;
}

auto App::init() -> std::expected<void, std::string> {
    auto* handle = window_->get_handle();

    int fb_width, fb_height;
    glfwGetFramebufferSize(handle, &fb_width, &fb_height);
    camera.aspect = static_cast<float>(fb_width) / static_cast<float>(fb_height);
    camera.resize(fb_width, fb_height);

    auto gz_result = render::GizmoRenderer::create();
    if (!gz_result) {
        std::println(stderr, "gizmo renderer error: {}", gz_result.error());
    } else {
        gizmo_renderer_ = std::make_unique<render::GizmoRenderer>(std::move(*gz_result));
    }

    auto sk_result = render::SkeletonRenderer::create();
    if (!sk_result) {
        std::println(stderr, "skeleton renderer error: {}", sk_result.error());
    } else {
        skeleton_renderer_ = std::make_unique<render::SkeletonRenderer>(std::move(*sk_result));
    }

    stage_ = std::make_unique<render::Stage>(100.0f, 20);

    auto sm_result = render::ShadowMap::create(2048, 2048);
    if (!sm_result) {
        std::println(stderr, "shadow map error: {}", sm_result.error());
    } else {
        shadow_map_ = std::move(*sm_result);
    }

    auto load_result = load_model(model_path);
    if (!load_result) {
        std::println(stderr, "model load error: {}", load_result.error());
    }
    auto motion_result = load_motion(motion_path);
    if (!motion_result) {
        std::println(stderr, "motion load error: {}", motion_result.error());
    }

    auto ui_result = ui::UiRenderer::init(handle, "c:\\Windows\\Fonts\\msyh.ttc");
    if (!ui_result) {
        return std::unexpected(ui_result.error());
    }
    ui_ = std::make_unique<ui::UiRenderer>(std::move(*ui_result));

    glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    std::println("--- MMD Model Viewer Controls ---");
    std::println("Mouse:");
    std::println("  Left Drag:   Rotate camera");
    std::println("  Right Drag:  Pan camera");
    std::println("  Scroll:      Zoom in/out\n");
    std::println("Keyboard:");
    std::println("  W, A, S, D:  Move camera");
    std::println("  R:           Reset all transformations");
    std::println("  C:           Toggle control window");
    std::println("  ESC:         Exit");
    std::println("---------------------------------");

    last_frame_time_ = static_cast<float>(window_->get_time());
    return {};
}

auto App::load_model(const std::string& path) -> std::expected<void, std::string> {
    auto result = Model::load(path);
    if (!result) return std::unexpected(result.error());
    model_ = std::move(*result);
    return {};
}

auto App::load_motion(const std::string& path) -> std::expected<void, std::string> {
    if (!model_) return std::unexpected("No model loaded");
    return model_->load_motion(path);
}

auto App::load_stage_pmx(const std::string& path) -> std::expected<void, std::string> {
    return stage_->load_pmx(path);
}

void App::reset_stage() {
    stage_->use_default_grid();
}

void App::reset() {
    if (model_) model_->reset_transform();
    camera.reset();
}

void App::run() {
    glfwSetWindowUserPointer(window_->get_handle(), this);

    timer::TimerResolution timer_res;

    while (!window_->should_close()) {
        float cur_time = static_cast<float>(window_->get_time());
        float dt = cur_time - last_frame_time_;
        last_frame_time_ = cur_time;

        if (model_) {
            model_->enable_motion = enable_motion;
            model_->is_playing = is_playing;
            model_->manual_bone_control = manual_bone_control;
            model_->update_anim(dt);
            model_->update_bones();
        }

        ui_->begin_frame();
        render_ui();
        render_gizmos();
        ui_->end_frame();

        auto light_space = compute_light_space();

        render_shadow_pass(light_space);
        render_main_pass(light_space);

        ui_->draw_frame();

        window_->swap_buffers();
        window_->poll_events();

        if (limit_fps && target_fps > 0) {
            float frame_time = static_cast<float>(window_->get_time()) - last_frame_time_;
            float target = 1.0f / static_cast<float>(target_fps);
            if (frame_time < target) {
                timer::sleep_for(target - frame_time);
            }
        }
    }
}

auto App::compute_light_space() const -> glm::mat4 {
    float near_plane = 1.0f, far_plane = 100.0f;
    glm::vec3 light_pos(-2, 4, -1);
    glm::mat4 light_proj;

    if (!lights.empty()) {
        if (lights[0].type == LIGHT_DIRECTIONAL) {
            light_pos = -lights[0].direction * 20.0f;
            light_proj = glm::ortho(-20.0f, 20.0f, -20.0f, 20.0f, near_plane, far_plane);
        } else {
            light_pos = lights[0].position;
            light_proj = glm::perspective(glm::radians(90.0f),
                static_cast<float>(shadow_map_.width()) / static_cast<float>(shadow_map_.height()),
                near_plane, far_plane);
        }
    } else {
        light_proj = glm::ortho(-20.0f, 20.0f, -20.0f, 20.0f, near_plane, far_plane);
    }

    glm::mat4 light_view = glm::lookAt(light_pos, glm::vec3(0), glm::vec3(0, 1, 0));
    return light_proj * light_view;
}

void App::render_shadow_pass(const glm::mat4& light_space) {
    shadow_map_.bind_write();

    if (show_stage && stage_) stage_->draw_shadow(light_space);

    if (model_) model_->draw_shadow(light_space);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void App::render_main_pass(const glm::mat4& light_space) {
    int win_w, win_h;
    glfwGetFramebufferSize(window_->get_handle(), &win_w, &win_h);
    glViewport(0, 0, win_w, win_h);
    glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (show_stage && stage_) {
        stage_->draw(camera, lights, ambient_color, ambient_strength, shadow_map_.texture(), light_space);
    }

    if (model_) {
        model_->draw(camera, lights, ambient_color, ambient_strength, shadow_map_.texture(), light_space);
    }

    if (show_light_gizmos && gizmo_renderer_) {
        gizmo_renderer_->draw_lights(camera, lights);
    }

    if (show_skeleton && skeleton_renderer_ && model_ && !model_->data().bone_defs.empty()) {
        const auto& states = model_->skeleton().states();
        std::vector<glm::mat4> globals;
        globals.reserve(states.size());
        for (const auto& st : states) globals.push_back(st.global_transform);
        skeleton_renderer_->draw(camera, model_->model_matrix(),
                                 model_->data().bone_defs, globals, selected_bone_index);
    }
}

void App::render_ui() {
    if (!show_control_window) return;

    ImGui::Begin("MMD Viewer Controls");
    if (ImGui::BeginTabBar("ControlTabs")) {
        if (ImGui::BeginTabItem("General")) {
            render_general_panel(*this);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Model")) {
            render_model_panel(*this, window_->get_handle());
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Skeleton")) {
            render_skeleton_panel(*this);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Stage")) {
            render_stage_panel(*this, window_->get_handle());
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Lighting")) {
            render_lighting_panel(*this);
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    ImGui::End();
}

void App::render_gizmos() {
    auto& style = ImGuizmo::GetStyle();
    style.RotationLineThickness = 4.0f;
    style.RotationOuterLineThickness = 5.0f;

    ImGuizmo::SetOrthographic(false);
    ImGuizmo::SetDrawlist(ImGui::GetForegroundDrawList());
    const auto& io = ImGui::GetIO();
    ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);

    glm::mat4 view = camera.get_view_matrix();
    glm::mat4 proj = camera.get_projection_matrix();

    if (model_ && manual_bone_control && !model_->data().bone_defs.empty()
        && selected_bone_index >= 0) {
        selected_light_index = -1;

        auto& defs = model_->data().bone_defs;
        auto& states = model_->skeleton().states();
        if (selected_bone_index < std::ssize(states)) {
            auto& st = states[selected_bone_index];
            const auto& def = defs[selected_bone_index];
            glm::mat4 mm = model_->model_matrix();
            glm::mat4 global = mm * st.global_transform;

            if (ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(proj),
                                     gizmo_op, gizmo_mode, glm::value_ptr(global))) {
                glm::mat4 parent_global(1);
                if (def.parent_index != -1) {
                    parent_global = states[def.parent_index].global_transform;
                }
                glm::mat4 parent_world = mm * parent_global;
                glm::mat4 local_transform = glm::inverse(parent_world) * global;

                glm::vec3 t = glm::vec3(local_transform[3]);
                glm::quat r = glm::quat_cast(glm::mat3(local_transform));

                glm::vec3 parent_pos(0);
                if (def.parent_index != -1) parent_pos = defs[def.parent_index].position;
                glm::vec3 relative_pos = def.position - parent_pos;

                st.local_translation = t - relative_pos;
                st.local_rotation = r;
            }
        }
    }

    if (show_light_gizmos && selected_light_index >= 0 && selected_light_index < std::ssize(lights)) {
        auto& light = lights[selected_light_index];
        glm::mat4 transform(1);

        if (light.type == LIGHT_POINT) {
            transform = glm::translate(transform, light.position);
        } else {
            glm::vec3 display_pos(0, 10, 0);
            transform = glm::translate(transform, display_pos);
            glm::vec3 default_dir(0, -1, 0);
            glm::vec3 from = glm::normalize(default_dir);
            glm::vec3 to = glm::normalize(light.direction);
            float d = glm::dot(from, to);
            glm::quat q;
            if (d > 0.9999f) {
                q = glm::quat(1, 0, 0, 0);
            } else if (d < -0.9999f) {
                glm::vec3 axis = glm::cross(glm::vec3(1, 0, 0), from);
                if (glm::length(axis) < 0.0001f) axis = glm::cross(glm::vec3(0, 1, 0), from);
                q = glm::angleAxis(glm::pi<float>(), glm::normalize(axis));
            } else {
                q = glm::angleAxis(std::acos(d), glm::normalize(glm::cross(from, to)));
            }
            transform = transform * glm::mat4_cast(q);
        }

        auto op = (light.type == LIGHT_DIRECTIONAL) ? ImGuizmo::ROTATE : ImGuizmo::TRANSLATE;

        if (ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(proj),
                                 op, ImGuizmo::WORLD, glm::value_ptr(transform))) {
            if (light.type == LIGHT_POINT) {
                light.position = glm::vec3(transform[3]);
            } else {
                glm::mat3 rot_mat(transform);
                light.direction = glm::normalize(rot_mat * glm::vec3(0, -1, 0));
            }
        }
    }
}


auto App::self(GLFWwindow* w) -> App& {
    return *static_cast<App*>(glfwGetWindowUserPointer(w));
}

void App::on_key(GLFWwindow* w, int key, int /*s*/, int action, int /*m*/) {
    auto& app = self(w);
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        app.camera.handle_keys(key, action);
        switch (key) {
            case GLFW_KEY_R: app.reset(); break;
            case GLFW_KEY_C: app.show_control_window = !app.show_control_window; break;
            case GLFW_KEY_ESCAPE: glfwSetWindowShouldClose(w, GLFW_TRUE); break;
            default: break;
        }
    }
}

void App::on_mouse_btn(GLFWwindow* w, int button, int action, int /*mods*/) {
    auto& app = self(w);
    if (action == GLFW_PRESS) {
        if (ImGui::GetCurrentContext() && ImGui::GetIO().WantCaptureMouse) return;
        glfwGetCursorPos(w, &app.last_x_, &app.last_y_);
        if (button == GLFW_MOUSE_BUTTON_LEFT) app.mouse_left_ = true;
        if (button == GLFW_MOUSE_BUTTON_RIGHT) app.mouse_right_ = true;
        if (button == GLFW_MOUSE_BUTTON_MIDDLE) app.mouse_middle_ = true;
    } else if (action == GLFW_RELEASE) {
        if (button == GLFW_MOUSE_BUTTON_LEFT) app.mouse_left_ = false;
        if (button == GLFW_MOUSE_BUTTON_RIGHT) app.mouse_right_ = false;
        if (button == GLFW_MOUSE_BUTTON_MIDDLE) app.mouse_middle_ = false;
    }
}

void App::on_cursor(GLFWwindow* w, double x, double y) {
    auto& app = self(w);
    double dx = x - app.last_x_;
    double dy = y - app.last_y_;
    if (app.mouse_left_)  app.camera.orbit(static_cast<float>(dx * 0.4), static_cast<float>(dy * 0.4));
    if (app.mouse_right_) app.camera.pan(static_cast<float>(dx * 0.05), static_cast<float>(dy * 0.05));
    app.last_x_ = x;
    app.last_y_ = y;
}

void App::on_scroll(GLFWwindow* w, double /*xoff*/, double yoff) {
    auto& app = self(w);
    if (ImGui::GetCurrentContext() && ImGui::GetIO().WantCaptureMouse) return;
    app.camera.handle_scroll(yoff);
}

void App::on_resize(GLFWwindow* w, int width, int height) {
    auto& app = self(w);
    if (height == 0) height = 1;
    glViewport(0, 0, width, height);
    app.camera.resize(width, height);
}
