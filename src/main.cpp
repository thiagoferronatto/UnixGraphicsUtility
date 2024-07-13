#include "external/gl.h"
#include <GLFW/glfw3.h>
#include "external/glm/glm.hpp"

int main() {
  if (!glfwInit()) return 1;

  auto window = glfwCreateWindow(800, 600, "Unix Graphics Utility", nullptr, nullptr);

  glfwMakeContextCurrent(window);

  glfwShowWindow(window);

  glClearColor(.1, .1, .1, 1);

  constexpr glm::vec3 vertices[]{
    {-0.5, -0.5, 0},
    { 0.5, -0.5, 0},
    {   0,  0.5, 0}
  };

  constexpr glm::vec3 colors[]{
    {1, 0, 0},
    {0, 1, 0},
    {0, 0, 1}
  };

  GLuint vertex_array;
  glGenVertexArrays(1, &vertex_array);
  glBindVertexArray(vertex_array);

  GLuint buffers[2];
  glGenBuffers(2, buffers);
  
  glBindBuffer(GL_ARRAY_BUFFER, buffers[0]);
  glBufferData(GL_ARRAY_BUFFER, 3 * 3 * sizeof(float), vertices, GL_STATIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
  glEnableVertexAttribArray(0);

  glBindBuffer(GL_ARRAY_BUFFER, buffers[1]);
  glBufferData(GL_ARRAY_BUFFER, 3 * 3 * sizeof(float), colors, GL_STATIC_DRAW);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
  glEnableVertexAttribArray(1);

  constexpr auto vertex_shader_source = R"(
    #version 460

    layout (location = 0) in vec3 vertex_position;
    layout (location = 1) in vec3 vertex_color;

    out vec3 color;

    void main(void) {
      gl_Position = vec4(vertex_position, 1);
      color = vertex_color;
    }
  )";

  auto vertex_shader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertex_shader, 1, &vertex_shader_source, nullptr);
  glCompileShader(vertex_shader);

  constexpr auto fragment_shader_source = R"(
    #version 460

    in vec3 color;

    out vec4 fragment_color;

    void main(void) {
      fragment_color = vec4(color, 1);
    }
  )";

  auto fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragment_shader, 1, &fragment_shader_source, nullptr);
  glCompileShader(fragment_shader);

  auto program = glCreateProgram();
  glAttachShader(program, vertex_shader);
  glAttachShader(program, fragment_shader);
  glLinkProgram(program);
  glUseProgram(program);

  while (!glfwWindowShouldClose(window)) {
    
    glClear(GL_COLOR_BUFFER_BIT);

    glDrawArrays(GL_TRIANGLES, 0, 3);

    glfwSwapBuffers(window);
    glfwPollEvents();
  }
  
  return 0;
}