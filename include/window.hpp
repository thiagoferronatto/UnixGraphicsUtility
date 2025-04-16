#ifndef GL_BOILERPLATE_HPP
#define GL_BOILERPLATE_HPP

// clang-format off
#include "external/gl.h"
#include "GLFW/glfw3.h"
// clang-format on
#include "external/glm/vec3.hpp"

#include <functional>
#include <stdexcept>
#include <thread>

class Window {
public:
  Window(size_t width, size_t height, const char *title);

  ~Window();

  size_t width() const;
  size_t height() const;

  bool shouldClose() const;
  void swapBuffers() const;
  void pollEvents() const;
  bool keyIsPressed(int key) const;
  std::tuple<float, float> getCursorPos() const;
  void show() const;

private:
  GLFWwindow *_window{};
  size_t _width, _height;
};

#endif // GL_BOILERPLATE_HPP
