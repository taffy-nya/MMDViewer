#pragma once
#include <expected>
#include <string>

struct GLFWwindow;

namespace ui {

class UiRenderer {
public:
    static auto init(GLFWwindow* window, const std::string& font_path)
        -> std::expected<UiRenderer, std::string>;

    ~UiRenderer();
    UiRenderer(UiRenderer&&) noexcept;
    UiRenderer& operator=(UiRenderer&&) noexcept;
    UiRenderer(const UiRenderer&) = delete;
    UiRenderer& operator=(const UiRenderer&) = delete;

    void begin_frame();
    void end_frame();
    void draw_frame();

private:
    UiRenderer() = default;
    bool moved_{false};
};

} // namespace ui
