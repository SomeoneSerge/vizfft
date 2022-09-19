#ifndef _IM_RAII_H
#define _IM_RAII_H

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <implot.h>
#include <iostream>
#include <stdexcept>

namespace ImRAII {
// FIXME:
constexpr double WINDOW_MIN_WIDTH = 1980;

class NoCopy {
public:
  NoCopy() = default;
  virtual ~NoCopy() = default;
  NoCopy(NoCopy &&) = delete; /* let's delete the moves for now as well */
  NoCopy(const NoCopy &) = delete;
  NoCopy operator=(const NoCopy &) = delete;
};

class SafeGlfwCtx : NoCopy {
public:
  SafeGlfwCtx() {
    if (!glfwInit()) {
      throw std::runtime_error("glfwInit() failed");
    }
  }
  ~SafeGlfwCtx() { glfwTerminate(); }
};

class SafeGlfwWindow : NoCopy {
public:
  SafeGlfwWindow(const char *title = "imraii.h") {
    const auto width = WINDOW_MIN_WIDTH;
    const auto height = WINDOW_MIN_WIDTH * (9.0 / 16.0) * .5;
    glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

#ifdef __EMSCRIPTEN__
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#endif

    _window = glfwCreateWindow(width, height, title, nullptr, nullptr);
  }
  ~SafeGlfwWindow() { glfwDestroyWindow(_window); }

  GLFWwindow *window() const { return _window; }
  void makeContextCurrent() const { glfwMakeContextCurrent(_window); };

private:
  GLFWwindow *_window;
};

class SafeGlew : NoCopy {
public:
  SafeGlew() {
    glewExperimental = GL_TRUE;
    glewInit();

    if (glGenBuffers == nullptr) {
      throw std::runtime_error("glewInit() failed: glGenBuffers == nullptr");
    }
  }
};

class SafeImGui : NoCopy {
public:
  SafeImGui(GLFWwindow *window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init();
    ImPlot::CreateContext();
    ImGui::StyleColorsDark();
  }
  ~SafeImGui() {
    ImPlot::DestroyContext();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
  }
};

class SafeVBO : NoCopy {
public:
  SafeVBO(GLsizeiptr size, const void *data, GLenum target = GL_ARRAY_BUFFER,
          GLenum usage = GL_STATIC_DRAW);

  ~SafeVBO();

  GLuint vbo() const;

private:
  GLuint _vbo;
};

class SafeVAO : NoCopy {
public:
  SafeVAO() {
    glGenVertexArrays(1, &_vao);
    bind();
  }

  ~SafeVAO() { glDeleteVertexArrays(1, &_vao); }

  GLuint vao() const { return _vao; }
  void bind() { glBindVertexArray(_vao); }

private:
  GLuint _vao;
};

class SafeShader : NoCopy {
public:
  SafeShader(GLenum shaderType, const char *source);
  ~SafeShader();

  GLuint shader() const;

private:
  GLuint _shader;
};

class SafeShaderProgram : NoCopy {
public:
  SafeShaderProgram() {
    /* we could do the linking to shaders, etc, right here
     * but atm we only care about the order of initialization
     * (and destruction)
     */
    _program = glCreateProgram();
  }

  ~SafeShaderProgram() { glDeleteProgram(_program); }

  GLuint program() const { return _program; }

private:
  GLuint _program;
};

class VtxFragProgram : NoCopy {
public:
  VtxFragProgram(const char *vtxShader, const char *fragShader);

  GLuint vtxShader() const;
  GLuint fragShader() const;
  GLuint program() const;

protected:
  SafeShader _vtxShader;
  SafeShader _fragShader;
  SafeShaderProgram _program;
};

class GlfwFrame : NoCopy {
public:
  GlfwFrame(GLFWwindow *window) : window(window) {
    glClearColor(.45f, .55f, .6f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
      glfwSetWindowShouldClose(window, GL_TRUE);
    }
  }
  ~GlfwFrame() {
    int dispWidth, dispHeight;
    glfwGetFramebufferSize(window, &dispWidth, &dispHeight);
    glViewport(0, 0, dispWidth, dispHeight);
    glfwSwapBuffers(window);
    glfwPollEvents();
  }

private:
  GLFWwindow *window;
};

class ImGuiGlfwFrame : NoCopy {
public:
  ImGuiGlfwFrame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
  }
  ~ImGuiGlfwFrame() {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
  }
};

// class SafeGlTexture : NoCopy {
// public:
//   SafeGlTexture(const Uint8Image &image,
//                 const unsigned int interpolation = GL_LINEAR,
//                 const unsigned int dtype = GL_UNSIGNED_BYTE)
//       : data({.texture = 0,
//               .xres = image.xres,
//               .yres = image.yres,
//               .channels = image.channels,
//               .dtype = dtype,
//               .interpolation = interpolation}) {
//     glGenTextures(1, &data.texture);
//     glBindTexture(GL_TEXTURE_2D, data.texture);
//
//     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, interpolation);
//     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, interpolation);
//     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
//     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
//
//     reallocate(image.channels, image.yres, image.xres, interpolation, dtype,
//                image.data.get());
//   }
//
//   SafeGlTexture(SafeGlTexture &&other) : data(other.data) {
//     other.data.texture = GL_INVALID_VALUE;
//   }
//
//   ~SafeGlTexture();
//
//   GLuint texture() const { return data.texture; }
//   void *textureVoidStar() const { return (void *)(intptr_t)data.texture; }
//   unsigned int reallocate(const int channels, const int height, const int
//   width,
//                           const unsigned int interpolation = GL_LINEAR,
//                           const unsigned int dtype = GL_UNSIGNED_BYTE,
//                           const void *imageData = nullptr) {
//
//     this->data = {.texture = this->data.texture,
//                   .xres = width,
//                   .yres = height,
//                   .channels = channels,
//                   .dtype = dtype,
//                   .interpolation = interpolation};
//
//     constexpr unsigned int modes[] = {0, GL_DEPTH_COMPONENT, 0, GL_RGB,
//                                       GL_RGBA};
//     if (channels != 1 && channels != 3 && channels != 4) {
//       std::cerr << "[E] texture cannot have " << channels << " channels"
//                 << std::endl;
//       throw std::invalid_argument("invalid number of channels");
//     }
//     const auto mode = modes[channels];
//     glTexImage2D(GL_TEXTURE_2D, 0, mode, data.xres, data.yres, 0, mode,
//     dtype,
//                  imageData);
//     return texture();
//   }
//   void bind() { glBindTexture(GL_TEXTURE_2D, data.texture); }
//   int xres() const { return data.xres; }
//   int yres() const { return data.yres; }
//   double aspect() const { return data.yres * 1.0 / data.xres; }
//
// private:
//   struct {
//     GLuint texture;
//     int xres, yres, channels;
//     unsigned int dtype;
//     unsigned int interpolation;
//   } data;
// };
}; // namespace ImRAII

#endif
