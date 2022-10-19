#ifndef _IM_RAII_H
#define _IM_RAII_H

#include <Eigen/Dense>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <cstring>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <implot.h>
#include <iomanip>
#include <iostream>
#include <memory>
#include <optional>
#include <stdexcept>

namespace ImRAII {
// FIXME:
constexpr double WINDOW_MIN_WIDTH = 1920;

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
  SafeGlfwWindow(const char *title = "imraii.h",
                 const double width = WINDOW_MIN_WIDTH,
                 const double height = WINDOW_MIN_WIDTH * (9.0 / 16.0),
                 const double device_pixel_ratio = 1.0) {
    glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

#ifdef __EMSCRIPTEN__
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#endif

    _window =
        glfwCreateWindow(width * device_pixel_ratio,
                         height * device_pixel_ratio, title, nullptr, nullptr);
#ifdef __EMSCRIPTEN__
    auto _result = emscripten_set_canvas_element_size("canvas", width, height);
#endif
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
    glfwGetFramebufferSize(window, &_width, &_height);

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

  int width() const { return _width; }
  int height() const { return _height; }

private:
  GLFWwindow *window;
  int _width, _height;
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

enum DType { u8, i32, f16, f32 };

template <typename Scalar> DType runtime_type();

template <> DType runtime_type<uint8_t>() { return u8; }
template <> DType runtime_type<Eigen::bfloat16>() { return f16; }
template <> DType runtime_type<int32_t>() { return i32; }
template <> DType runtime_type<float>() { return f32; }

int dtype_itemsize(DType dtype) {
  switch (dtype) {
  case u8:
    return 1;
  case f16:
    return 2;
  case i32:
  case f32:
    return 4;
  default:
    std::abort();
  }
}

GLenum dtype_to_gl(DType dtype) {
  switch (dtype) {
  case u8:
    return GL_UNSIGNED_BYTE;
  case f32:
    return GL_FLOAT;
  default:
    std::abort();
  }
}

class SafeGlTexture : NoCopy {
public:
  SafeGlTexture(const unsigned char *image = nullptr, const int rows = 480,
                const int cols = 640, const int channels = 1,
                const DType dtype = f32,
                const unsigned int filter_min = GL_LINEAR,
                const unsigned int filter_mag = GL_NEAREST)
      : data({
            .texture = 0,
            .rows = rows,
            .cols = cols,
            .channels = channels,
            .dtype = dtype,
            .filter_min = filter_min,
            .filter_mag = filter_mag,
        }) {
    glGenTextures(1, &data.texture);

    reallocate(channels, data.rows, data.cols, dtype, filter_min, filter_mag,
               image);
  }

  SafeGlTexture(SafeGlTexture &&other) : data(other.data) {
    other.data.texture = GL_INVALID_VALUE;
  }

  ~SafeGlTexture() {
    if (data.texture != GL_INVALID_VALUE) {
      glDeleteTextures(1, &data.texture);
    }
  }

  GLuint texture() const { return data.texture; }
  void *textureVoidStar() const { return (void *)(intptr_t)data.texture; }
  unsigned int reallocate(const int channels, const int rows, const int cols,
                          const DType dtype,
                          const unsigned int filter_min = GL_LINEAR,
                          const unsigned int filter_mag = GL_NEAREST,
                          const void *imageData = nullptr) {

    this->data = {.texture = this->data.texture,
                  .rows = rows,
                  .cols = cols,
                  .channels = channels,
                  .dtype = dtype,
                  .filter_min = filter_min,
                  .filter_mag = filter_mag};

    constexpr GLint modes[] = {0, GL_RED, 0, GL_RGB, GL_RGBA};
    if (channels != 1 && channels != 3 && channels != 4) {
      std::cerr << "[E] texture cannot have " << channels << " channels"
                << std::endl;
      throw std::invalid_argument("invalid number of channels");
    }
    const auto mode = modes[channels];

    glBindTexture(GL_TEXTURE_2D, data.texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter_min);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter_mag);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    const auto width = data.cols;
    const auto height = data.rows;
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, width, height, 0, GL_RED, GL_FLOAT,
                 imageData);

    glBindTexture(GL_TEXTURE_2D, 0);
    return texture();
  }

  void bind() { glBindTexture(GL_TEXTURE_2D, data.texture); }
  int rows() const { return data.rows; }
  int cols() const { return data.cols; }
  double aspect() const { return data.rows * 1.0 / data.cols; }

private:
  struct {
    GLuint texture;
    int rows, cols, channels;
    unsigned int dtype;
    unsigned int filter_min;
    unsigned int filter_mag;
  } data;
};

struct ImBeginWindow {
  bool visible;

  ImBeginWindow(const char *name = "An imgui window", bool *p_open = nullptr,
                ImGuiWindowFlags_ flags = ImGuiWindowFlags_None) {
    visible = ImGui::Begin(name, p_open, flags);
  }
  ~ImBeginWindow() { ImGui::End(); }
};

struct ImBeginPlot {
  bool visible;

  ImBeginPlot(const char *title_id = "An imgui window",
              const ImVec2 &size = ImVec2(-1, 0),
              ImPlotFlags_ flags = ImPlotFlags_None) {
    visible = ImPlot::BeginPlot(title_id, size, flags);
  }
  ~ImBeginPlot() {
    if (visible) {
      ImPlot::EndPlot();
    }
  }
};

struct ImPushId {
  ImPushId(const char *id) { ImGui::PushID(id); }
  ImPushId(int id) { ImGui::PushID(id); }
  ~ImPushId() { ImGui::PopID(); }
};

struct ImEditVec2 {
  struct Vec2Data {
    ImVec2 mouseStart;
    ImVec2 valueStart;
  };

  std::optional<Vec2Data> edit;
  ImVec2 boundsMin, boundsMax;
  const char *id;
  std::ostringstream ss;
  std::string s;

  ImEditVec2(const char *id, ImVec2 boundsMin, ImVec2 boundsMax)
      : boundsMin(boundsMin), boundsMax(boundsMax), id(id) {}

  bool dragging() const { return edit.has_value(); }

  bool show(float &x, float &y) {
    ImPushId _id(id);
    ss.str("");
    ss << "[" << std::setfill('0') << std::setw(4) << std::setprecision(1)
       << std::fixed << x << "]" << std::endl
       << "[" << std::setfill('0') << std::setw(4) << std::setprecision(1)
       << std::fixed << y << "]";
    s = ss.str();

    const auto visible = ImGui::Button(s.c_str());

    const auto click = ImGui::IsItemClicked();
    const auto mouseDown = ImGui::IsMouseDown(0);
    const auto coords = ImGui::GetMousePos();

    if (click and not dragging()) {
      edit = {coords, ImVec2(x, y)};
    }

    if (dragging() and not mouseDown) {
      edit.reset();
    }

    if (dragging()) {
      const auto dx = (coords.x - edit->mouseStart.x);
      const auto dy = -(coords.y - edit->mouseStart.y);
      constexpr auto speed = 0.25f;
      x = edit->valueStart.x + speed * dx;
      y = edit->valueStart.y + speed * dy;
    }
    return visible;
  }

  ~ImEditVec2() {}
};

}; // namespace ImRAII

#endif
