#include "app/app.h"
#include "platform/file_dialog.h"
#include "imgui.h"
#include <print>

void render_model_panel(App& app, GLFWwindow* win) {
    ImGui::Text("Current Model: %s", app.model_path.c_str());
    if (ImGui::Button("Load Model (.pmx)")) {
        auto path = file_dialog::open("PMX Files", "*.pmx", win);
        if (!path) {
            std::println(stderr, "dialog error: {}", path.error());
        } else if (!path->empty()) {
            app.model_path = *path;
            auto result = app.load_model(app.model_path);
            if (!result) std::println(stderr, "model load error: {}", result.error());
        }
    }
    ImGui::Text("Current Motion: %s", app.motion_path.c_str());
    if (ImGui::Button("Load Motion (.vmd)")) {
        auto path = file_dialog::open("VMD Files", "*.vmd", win);
        if (!path) {
            std::println(stderr, "dialog error: {}", path.error());
        } else if (!path->empty()) {
            app.motion_path = *path;
            auto result = app.load_motion(app.motion_path);
            if (!result) std::println(stderr, "motion load error: {}", result.error());
        }
    }

    ImGui::Separator();
    ImGui::Text("Transform");
    {
        auto pos = app.translation();
        if (ImGui::DragFloat3("##Pos", &pos.x, 0.1f)) app.set_translation(pos);
        ImGui::SameLine();
        if (ImGui::Button("Reset##Pos")) app.set_translation(glm::vec3(0));
        ImGui::SameLine();
        ImGui::Text("Position");

        auto rot = app.rotation();
        if (ImGui::DragFloat3("##Rot", &rot.x, 1.0f)) app.set_rotation(rot);
        ImGui::SameLine();
        if (ImGui::Button("Reset##Rot")) app.set_rotation(glm::vec3(0));
        ImGui::SameLine();
        ImGui::Text("Rotation");

        auto scl = app.scale();
        if (ImGui::DragFloat3("##Scale", &scl.x, 0.01f)) app.set_scale(scl);
        ImGui::SameLine();
        if (ImGui::Button("Reset##Scale")) app.set_scale(glm::vec3(1));
        ImGui::SameLine();
        ImGui::Text("Scale");
    }

    ImGui::Separator();
    ImGui::Text("Animation");
    ImGui::Checkbox("Enable Motion", &app.enable_motion);
    if (app.enable_motion) {
        ImGui::Checkbox("Play Animation", &app.is_playing);
    }
}
