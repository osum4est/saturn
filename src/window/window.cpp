#include "saturn/window/window.h"

#ifdef SATURN_MACOS
#define GLFW_EXPOSE_NATIVE_COCOA
#endif

#ifdef SATURN_LINUX
#define GLFW_EXPOSE_NATIVE_X11
// TODO: Wayland?
#endif

#include <glfw/glfw3.h>
#include <glfw/glfw3native.h>
#include <result/result.h>

namespace saturn {

window::window(uint32_t width, uint32_t height, GLFWwindow* window)
    : _width(width), _height(height), _window(window) { }

window::~window() {
    glfwDestroyWindow(_window);
}

result::ptr<window> window::create(const window_config& config) {
    if (!glfwInit()) return result::err("Failed to initialize glfw");

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    auto glfw_window = glfwCreateWindow(config.width, config.height, config.title.c_str(), nullptr, nullptr);
    if (!glfw_window) return result::err("Failed to create window");

    return result::ok(new window(config.width, config.height, glfw_window));
}

uint32_t window::width() const {
    return _width;
}

uint32_t window::height() const {
    return _height;
}

void* window::native_handle() const {
#ifdef SATURN_MACOS
    return glfwGetCocoaWindow(_window);
#endif
#ifdef SATURN_LINUX
    return glfwGetX11Window(_window);
#endif
    return nullptr;
}

bool window::should_close() const {
    return glfwWindowShouldClose(_window);
}

void window::poll_events() const {
    glfwPollEvents();
}

void window::resize(uint32_t width, uint32_t height) {
    _width = width;
    _height = height;
    glfwSetWindowSize(_window, width, height);
}

void window::title(const std::string& title) {
    glfwSetWindowTitle(_window, title.c_str());
}

} // namespace saturn