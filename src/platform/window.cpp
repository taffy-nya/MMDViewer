#include <glad/glad.h>
#include "window.h"

auto Window::create(const Cfg& cfg) -> std::expected<Window, std::string> {
    if (!glfwInit()) {
        return std::unexpected("Failed to initialize GLFW");
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, cfg.gl_major);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, cfg.gl_minor);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    Window w;
    w.p_win_ = glfwCreateWindow(cfg.width, cfg.height, cfg.title, nullptr, nullptr);
    if (!w.p_win_) {
        glfwTerminate();
        return std::unexpected("Failed to create GLFW window");
    }

    glfwMakeContextCurrent(w.p_win_);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        glfwDestroyWindow(w.p_win_);
        glfwTerminate();
        return std::unexpected("Failed to initialize GLAD");
    }

    glfwSwapInterval(cfg.vsync ? 1 : 0);

    return w;
}

Window::~Window() {
    if (p_win_) {
        glfwDestroyWindow(p_win_);
        p_win_ = nullptr;
        glfwTerminate();
    }
}

Window::Window(Window&& o) noexcept : p_win_(o.p_win_) { o.p_win_ = nullptr; }
Window& Window::operator=(Window&& o) noexcept {
    if (this != &o) { p_win_ = o.p_win_; o.p_win_ = nullptr; }
    return *this;
}

bool Window::should_close() const { return glfwWindowShouldClose(p_win_); }
void Window::set_should_close(bool v) { glfwSetWindowShouldClose(p_win_, v ? GLFW_TRUE : GLFW_FALSE); }
void Window::swap_buffers() { glfwSwapBuffers(p_win_); }
void Window::poll_events() { glfwPollEvents(); }
double Window::get_time() const { return glfwGetTime(); }
GLFWwindow* Window::get_handle() const { return p_win_; }
