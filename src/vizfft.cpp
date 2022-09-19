#ifdef __EMSCRIPTEN__
#include <emscripten.h>

#define GL_GLEXT_PROTOTYPES
#define EGL_EGLEXT_PROTOTYPES

#define GLFW_INCLUDE_ES3
#include <GLES3/gl3.h>
#endif

#include "imraii.h"
#include <functional>

using namespace ImRAII;

struct App {
  SafeGlfwCtx &glfwCtx;
  SafeGlfwWindow &glfwWindow;
  SafeGlew &glew;
  SafeImGui &imguiContext;

  void frame();
};

std::function<void()> frame_global;
void call_frame_global() { frame_global(); }

int main(int argc, char *argv[]) {
  SafeGlfwCtx glfwCtx;
  SafeGlfwWindow glfwWindow;
  glfwWindow.makeContextCurrent();

  SafeGlew glew;
  SafeImGui imguiContext(glfwWindow.window());

  std::cerr << "glGetString(GL_VERSION): " << glGetString(GL_VERSION)
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

void App::frame() {

  GlfwFrame glfwFrame(glfwWindow.window());
  ImGuiGlfwFrame imguiFrame;

  if (ImGui::Begin("hello")) {
    ImGui::Text("whatever2");
  }
  ImGui::End();
}
