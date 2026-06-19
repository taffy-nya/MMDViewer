#include "app/app.h"
#include "imgui.h"
#include "ImGuizmo.h"
#include <format>
#include <ranges>

void render_skeleton_panel(App& app) {
    ImGui::Checkbox("Show Skeleton", &app.show_skeleton);
    ImGui::Checkbox("Manual Control", &app.manual_bone_control);
    if (app.manual_bone_control) {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1, 1, 0, 1), "(Animation Paused)");
    }

    ImGui::Separator();

    if (!app.model().bone_defs.empty()) {
        if (ImGui::Button("Reset All Bones")) {
            app.skeleton().reset_pose();
            app.selected_bone_index = -1;
        }
    }

    auto& defs = app.model().bone_defs;
    if (!defs.empty()) {
        auto& states = app.skeleton().states();

        ImGui::BeginChild("BoneList", ImVec2(150, 0), true);
        for (auto&& [idx, def] : std::views::enumerate(defs)) {
            auto i = static_cast<int>(idx);
            auto label = std::format("{}: {}", i, def.name);
            if (ImGui::Selectable(label.c_str(), app.selected_bone_index == i)) {
                app.selected_bone_index = i;
            }
        }
        ImGui::EndChild();

        ImGui::SameLine();

        ImGui::BeginGroup();
        if (app.selected_bone_index >= 0 && app.selected_bone_index < std::ssize(defs)) {
            auto& st = states[app.selected_bone_index];
            const auto& def = defs[app.selected_bone_index];
            ImGui::Text("Bone: %s", def.name.c_str());
            ImGui::Text("ID: %d", app.selected_bone_index);
            ImGui::Text("Parent: %d", def.parent_index);

            ImGui::Separator();

            if (app.manual_bone_control) {
                if (ImGui::RadioButton("Rotate", app.gizmo_op == ImGuizmo::ROTATE))
                    app.gizmo_op = ImGuizmo::ROTATE;
                ImGui::SameLine();
                if (ImGui::RadioButton("Translate", app.gizmo_op == ImGuizmo::TRANSLATE))
                    app.gizmo_op = ImGuizmo::TRANSLATE;

                ImGui::Text("Local Translation");
                if (ImGui::DragFloat3("##BoneTrans", &st.local_translation.x, 0.01f)) {}
                if (ImGui::Button("Reset Trans")) {
                    st.local_translation = glm::vec3(0);
                }

                ImGui::Text("Local Rotation");
                glm::vec3 euler = glm::degrees(glm::eulerAngles(st.local_rotation));
                if (ImGui::DragFloat3("##BoneRot", &euler.x, 1.0f)) {
                    st.local_rotation = glm::quat(glm::radians(euler));
                }
                if (ImGui::Button("Reset Rot")) {
                    st.local_rotation = glm::quat(1, 0, 0, 0);
                }
            } else {
                ImGui::TextColored(ImVec4(1, 0, 0, 1), "Enable Manual Control to edit");
                ImGui::Text("Translation: %.2f, %.2f, %.2f",
                            st.local_translation.x, st.local_translation.y, st.local_translation.z);
                glm::vec3 euler = glm::degrees(glm::eulerAngles(st.local_rotation));
                ImGui::Text("Rotation: %.2f, %.2f, %.2f", euler.x, euler.y, euler.z);
            }
        } else {
            ImGui::Text("Select a bone to edit");
        }
        ImGui::EndGroup();
    } else {
        ImGui::Text("No model loaded");
    }
}
