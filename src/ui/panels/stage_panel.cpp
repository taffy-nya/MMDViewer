#include "app/app.h"
#include "platform/file_dialog.h"
#include "imgui.h"
#include <print>

void render_stage_panel(App& app, GLFWwindow* win) {
    ImGui::Checkbox("Show Stage", &app.show_stage);
    ImGui::Text("Current Stage: %s", app.stage_path.c_str());
    if (ImGui::Button("Load Stage PMX")) {
        auto path = file_dialog::open("PMX Files", "*.pmx", win);
        if (!path) {
            std::println(stderr, "dialog error: {}", path.error());
        } else if (!path->empty()) {
            app.stage_path = *path;
            auto result = app.load_stage_pmx(app.stage_path);
            if (!result) std::println(stderr, "stage load error: {}", result.error());
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset Stage")) {
        app.stage_path = "Default Grid";
        app.reset_stage();
    }
}
