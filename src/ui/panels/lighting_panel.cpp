#include "app/app.h"
#include "imgui.h"
#include <ranges>

void render_lighting_panel(App& app) {
    ImGui::Checkbox("Show Light Gizmos", &app.show_light_gizmos);
    ImGui::Text("Ambient Light");
    ImGui::ColorEdit3("Color", &app.ambient_color.x);
    ImGui::SliderFloat("Strength", &app.ambient_strength, 0.0f, 1.0f);

    ImGui::Separator();
    ImGui::Text("Dynamic Lights");
    if (ImGui::Button("Add Light")) {
        if (app.lights.size() < 16) {
            Light l;
            l.type = app.lights.empty() ? LIGHT_DIRECTIONAL : LIGHT_POINT;
            l.position = glm::vec3(0, 10, 0);
            app.lights.push_back(l);
        }
    }

    ImGui::Separator();

    if (app.selected_light_index != -1) {
        if (ImGui::Button("Deselect Light")) {
            app.selected_light_index = -1;
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset Selected Light")) {
            if (app.selected_light_index >= 0 && app.selected_light_index < static_cast<int>(app.lights.size())) {
                auto& l = app.lights[app.selected_light_index];
                l.color = glm::vec3(1);
                l.intensity = 1.0f;
                l.enabled = true;
                l.constant = 1.0f;
                l.linear = 0.09f;
                l.quadratic = 0.032f;
                if (l.type == LIGHT_POINT) l.position = glm::vec3(0, 10, 0);
                else l.direction = glm::normalize(glm::vec3(0, -1, 0));
            }
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset All Lights")) {
        for (auto& l : app.lights) {
            l.color = glm::vec3(1);
            l.intensity = 1.0f;
            l.enabled = true;
            l.constant = 1.0f;
            l.linear = 0.09f;
            l.quadratic = 0.032f;
            if (l.type == LIGHT_POINT) l.position = glm::vec3(0, 10, 0);
            else l.direction = glm::normalize(glm::vec3(0, -1, 0));
        }
    }

    for (auto i : std::views::iota(0, std::ssize(app.lights))) {
        ImGui::PushID(i);
        bool is_selected = (app.selected_light_index == i);
        ImGuiTreeNodeFlags flags = is_selected ? ImGuiTreeNodeFlags_Selected : 0;
        bool is_open = ImGui::TreeNodeEx(("Light " + std::to_string(i)).c_str(), flags);

        if (ImGui::IsItemClicked()) {
            app.selected_light_index = i;
            app.selected_bone_index = -1;
        }

        if (is_open) {
            auto& l = app.lights[i];
            ImGui::Checkbox("Enabled", &l.enabled);
            const char* types[] = {"Directional", "Point"};
            ImGui::Combo("Type", &l.type, types, 2);

            if (l.type == LIGHT_DIRECTIONAL) {
                float pitch = glm::degrees(asin(glm::clamp(-l.direction.y, -1.0f, 1.0f)));
                float yaw = glm::degrees(atan2(l.direction.x, l.direction.z));

                bool changed = false;
                changed |= ImGui::DragFloat("Pitch", &pitch, 1.0f, -89.0f, 89.0f);
                changed |= ImGui::DragFloat("Yaw", &yaw, 1.0f, -180.0f, 180.0f);

                if (changed) {
                    float rp = glm::radians(pitch);
                    float ry = glm::radians(yaw);
                    l.direction.x = sin(ry) * cos(rp);
                    l.direction.y = -sin(rp);
                    l.direction.z = cos(ry) * cos(rp);
                    l.direction = glm::normalize(l.direction);
                }
                ImGui::TextDisabled("Dir: (%.2f, %.2f, %.2f)", l.direction.x, l.direction.y, l.direction.z);
            } else {
                ImGui::DragFloat3("Position", &l.position.x, 0.5f);
                ImGui::DragFloat("Constant", &l.constant, 0.01f, 0.0f, 10.0f);
                ImGui::DragFloat("Linear", &l.linear, 0.001f, 0.0f, 1.0f);
                ImGui::DragFloat("Quadratic", &l.quadratic, 0.0001f, 0.0f, 1.0f);
            }

            ImGui::ColorEdit3("Color", &l.color.x);
            ImGui::SliderFloat("Intensity", &l.intensity, 0.0f, 5.0f);

            if (ImGui::Button("Remove")) {
                app.lights.erase(app.lights.begin() + i);
                if (app.selected_light_index == i) app.selected_light_index = -1;
                ImGui::TreePop();
                ImGui::PopID();
                continue;
            }

            ImGui::TreePop();
        }
        ImGui::PopID();
    }
}
