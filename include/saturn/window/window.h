#ifndef SATURN_WINDOW_H
#define SATURN_WINDOW_H

#include <result/result.h>
#include <string>

class GLFWwindow;

namespace saturn {

struct window_config {
    int width;
    int height;
    std::string title;
};

class window {
    uint32_t _width;
    uint32_t _height;
    GLFWwindow* _window;

    window(uint32_t width, uint32_t height, GLFWwindow* window);

  public:
    ~window();

    static result::ptr<window> create(const window_config& config);

    [[nodiscard]] uint32_t width() const;
    [[nodiscard]] uint32_t height() const;
    [[nodiscard]] void* native_handle() const;

    bool should_close() const;
    void poll_events() const;

    void resize(uint32_t width, uint32_t height);
    void title(const std::string& title);
};

} // namespace saturn

#endif
