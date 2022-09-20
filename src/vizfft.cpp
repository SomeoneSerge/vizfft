#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>

#define GL_GLEXT_PROTOTYPES
#define EGL_EGLEXT_PROTOTYPES

#define GLFW_INCLUDE_ES3
#include <GLES3/gl3.h>
#endif

#include "implot.h"
#include "imraii.h"
#include <functional>

using namespace ImRAII;

using FloatGrayscale =
    Eigen::Array<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;
using VectorXf = Eigen::Array<float, 1, Eigen::Dynamic, Eigen::RowMajor>;

struct App {
  SafeGlfwCtx &glfwCtx;
  SafeGlfwWindow &glfwWindow;
  SafeGlew &glew;
  SafeImGui &imguiContext;
  SafeGlTexture tex0;

  void frame();
};

void App::frame() {

  GlfwFrame glfwFrame(glfwWindow.window());
  ImGuiGlfwFrame imguiFrame;

  const auto cols = 640, rows = 320;
  const auto uu = VectorXf::LinSpaced(cols, 0, 1.0).colwise().replicate(rows);
  const auto vv =
      VectorXf::LinSpaced(rows, 0, 1.0).transpose().rowwise().replicate(cols);
  FloatGrayscale im = (10 * uu + 5 * vv).sin();

  tex0.reallocate(1, im.rows(), im.cols(), f32, GL_LINEAR, GL_NEAREST,
                  im.data());

  ImGui::SetNextWindowPos({10.0, 10.0}, ImGuiCond_Once);
  if (ImGui::Begin("hello")) {
    ImGui::Text("whatever2");
    ImGui::Text("%d %d", glfwFrame.width(), glfwFrame.height());

    if (ImPlot::BeginPlot("whateverPlot", {320, 240})) {
      ImPlot::PlotImage("whateverImage", tex0.textureVoidStar(), {0.0, 0.0},
                        {1.0, 1.0});
      ImPlot::EndPlot();
    }
  }
  ImGui::End();
}

std::function<void()> frame_global;
void call_frame_global() { frame_global(); }

int main(int argc, char *argv[]) {
  SafeGlfwCtx glfwCtx;
  SafeGlfwWindow glfwWindow;
  glfwWindow.makeContextCurrent();

  SafeGlew glew;
  SafeImGui imguiContext(glfwWindow.window());

  std::cout << "glGetString(GL_VERSION): " << glGetString(GL_VERSION)
            << std::endl;

  App app = {glfwCtx, glfwWindow, glew, imguiContext};

#ifdef __EMSCRIPTEN__
  frame_global = [&]() { app.frame(); };
  emscripten_set_main_loop(call_frame_global, 0, true);
#else
  while (!glfwWindowShouldClose(glfwWindow.window())) {
    app.frame();
  }
#endif

  return 0;
}
