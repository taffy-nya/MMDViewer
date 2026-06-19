#pragma once
#include <GLFW/glfw3.h>
#include <expected>
#include <string>

class Window {
public:
    struct Cfg {
        int width  = 1920;
        int height = 1080;
        const char* title = "MMDViewer";
        int gl_major = 3;
        int gl_minor = 3;
        bool vsync = true;
    };

    static auto create(const Cfg& cfg) -> std::expected<Window, std::string>;

    ~Window();
    Window(Window&&) noexcept;
    Window& operator=(Window&&) noexcept;
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    bool should_close() const;
    void set_should_close(bool v);
    void swap_buffers();
    void poll_events();
    double get_time() const;
    GLFWwindow* get_handle() const;

private:
    Window() = default;
    GLFWwindow* p_win_ = nullptr;
};
