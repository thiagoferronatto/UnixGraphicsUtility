#ifndef GL_UTIL_HPP
#define GL_UTIL_HPP

#ifdef NDEBUG
#define glCheck(call) call
#define glCheckShaderCompilation(shader) ((void)0)
#define glCheckProgramLinkage(program) ((void)0)
#else
#define glCheck(call)                                                          \
  {                                                                            \
    call;                                                                      \
    switch (glGetError()) {                                                    \
    case GL_NO_ERROR:                                                          \
      break;                                                                   \
    case GL_INVALID_ENUM:                                                      \
      fprintf(stderr, "%s\n", "GL_INVALID_ENUM after " #call);                 \
      std::terminate();                                                        \
    case GL_INVALID_VALUE:                                                     \
      fprintf(stderr, "%s\n", "GL_INVALID_VALUE after " #call);                \
      std::terminate();                                                        \
    case GL_INVALID_OPERATION:                                                 \
      fprintf(stderr, "%s\n", "GL_INVALID_OPERATION after " #call);            \
      std::terminate();                                                        \
    case GL_INVALID_FRAMEBUFFER_OPERATION:                                     \
      fprintf(stderr, "%s\n",                                                  \
              "GL_INVALID_FRAMEBUFFER_OPERATION after " #call);                \
      std::terminate();                                                        \
    case GL_OUT_OF_MEMORY:                                                     \
      fprintf(stderr, "%s\n", "GL_OUT_OF_MEMORY after " #call);                \
      std::terminate();                                                        \
    case GL_STACK_UNDERFLOW:                                                   \
      fprintf(stderr, "%s\n", "GL_STACK_UNDERFLOW after " #call);              \
      std::terminate();                                                        \
    case GL_STACK_OVERFLOW:                                                    \
      fprintf(stderr, "%s\n", "GL_STACK_OVERFLOW after " #call);               \
      std::terminate();                                                        \
    default:                                                                   \
      fprintf(stderr, "%s\n", "unknown OpenGL error after " #call);            \
      std::terminate();                                                        \
    }                                                                          \
  }                                                                            \
  (void)0

#define glCheckShaderCompilation(shader)                                       \
  {                                                                            \
    GLint isCompiled{};                                                        \
    glCheck(glGetShaderiv((shader), GL_COMPILE_STATUS, &isCompiled));          \
    if (isCompiled == GL_FALSE) {                                              \
      GLint maxLength{};                                                       \
      glCheck(glGetShaderiv((shader), GL_INFO_LOG_LENGTH, &maxLength));        \
      auto errorLog{new char[maxLength]};                                      \
      glCheck(glGetShaderInfoLog((shader), maxLength, &maxLength, errorLog));  \
      fprintf(stderr, "%s\n", errorLog);                                       \
      delete[] errorLog;                                                       \
      glCheck(glDeleteShader((shader)));                                       \
      std::terminate();                                                        \
    }                                                                          \
  }

#define glCheckProgramLinkage(program)                                         \
  {                                                                            \
    GLint isLinked{};                                                          \
    glCheck(glGetProgramiv(program, GL_LINK_STATUS, (int *)&isLinked));        \
    if (isLinked == GL_FALSE) {                                                \
      GLint maxLength{};                                                       \
      glCheck(glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength));        \
      auto infoLog{new char[maxLength]};                                       \
      glCheck(                                                                 \
          glGetProgramInfoLog(program, maxLength, &maxLength, &infoLog[0]));   \
      fprintf(stderr, "%s\n", infoLog);                                        \
      delete[] infoLog;                                                        \
      glCheck(glDeleteProgram(program));                                       \
      std::terminate();                                                        \
    }                                                                          \
  }
#endif // NDEBUG

#endif // GL_UTIL_HPP
