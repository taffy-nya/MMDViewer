#include "app/app.h"
#include "imgui.h"

void render_general_panel(App& app) {
    const auto& io = ImGui::GetIO();
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                1000.0f / io.Framerate, io.Framerate);
    ImGui::Separator();
    ImGui::Text("Performance");
    ImGui::Checkbox("Limit FPS", &app.limit_fps);
    if (app.limit_fps) {
        ImGui::SliderInt("Target FPS", &app.target_fps, 10, 240);
    }
    ImGui::Separator();
    ImGui::Text("Camera");
    if (ImGui::Button("Reset Camera")) {
        app.camera.reset();
    }
    ImGui::Separator();
    ImGui::Text("Background");
    ImGui::ColorEdit3("Clear Color", &app.clear_color.x);
}
