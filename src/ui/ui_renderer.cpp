#include "ui/ui_renderer.h"
#include <GLFW/glfw3.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "ImGuizmo.h"

namespace ui {

auto UiRenderer::init(GLFWwindow* window, const std::string& font_path) -> std::expected<UiRenderer, std::string> {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    if (!font_path.empty()) {
        io.Fonts->AddFontFromFileTTF(font_path.c_str(), 20.0f, nullptr,
                                     io.Fonts->GetGlyphRangesChineseFull());
    }
    ImGui::StyleColorsDark();

    if (!ImGui_ImplGlfw_InitForOpenGL(window, true)) {
        return std::unexpected("ImGui_ImplGlfw_InitForOpenGL failed");
    }

    constexpr const char* glsl_version = "#version 330";
    if (!ImGui_ImplOpenGL3_Init(glsl_version)) {
        return std::unexpected("ImGui_ImplOpenGL3_Init failed");
    }

    return UiRenderer{};
}

UiRenderer::~UiRenderer() {
    if (moved_) return;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

UiRenderer::UiRenderer(UiRenderer&& other) noexcept {
    other.moved_ = true;
}

auto UiRenderer::operator=(UiRenderer&& other) noexcept -> UiRenderer& {
    if (this != &other) other.moved_ = true;
    return *this;
}

void UiRenderer::begin_frame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGuizmo::BeginFrame();
}

void UiRenderer::end_frame() {
    ImGui::Render();
}

void UiRenderer::draw_frame() {
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

} // namespace ui
