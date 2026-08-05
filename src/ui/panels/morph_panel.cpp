#include "app/app.h"
#include "imgui.h"

static const char* panel_names[] = {
    "System", "Eyebrow", "Eye", "Lip", "Other"
};

void render_morph_panel(App& app) {
    if (!app.has_model()) {
        ImGui::Text("No model loaded.");
        return;
    }

    auto& morph_ctrl = app.model().morph_ctrl();
    const auto& morph_defs = app.model().data().morph_defs;

    if (morph_ctrl.morph_count() == 0) {
        ImGui::Text("No morphs in this model.");
        return;
    }

    if (ImGui::Button("Reset All Morphs")) {
        morph_ctrl.reset();
    }

    for (int panel = 1; panel <= 4; ++panel) {
        bool has_morphs = false;
        for (int i = 0; i < static_cast<int>(morph_defs.size()); ++i) {
            if (morph_defs[i].panel == panel) {
                has_morphs = true;
                break;
            }
        }
        if (!has_morphs) continue;

        if (ImGui::CollapsingHeader(panel_names[panel], ImGuiTreeNodeFlags_DefaultOpen)) {
            for (int i = 0; i < static_cast<int>(morph_defs.size()); ++i) {
                if (morph_defs[i].panel != panel) continue;

                const auto& def = morph_defs[i];
                float w = morph_ctrl.target_weight(i);
                if (ImGui::SliderFloat(def.name.c_str(), &w, 0.0f, 1.0f)) {
                    morph_ctrl.set_target_weight(i, w);
                }
            }
        }
    }
}
