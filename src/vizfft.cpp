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
#include <unsupported/Eigen/FFT>

using namespace ImRAII;

using FloatGrayscale =
    Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;
using VectorXf = Eigen::Matrix<float, 1, Eigen::Dynamic, Eigen::RowMajor>;

using MatrixXYcf = Eigen::Matrix<std::complex<float>, Eigen::Dynamic,
                                 Eigen::Dynamic, Eigen::RowMajor>;
using Matrix1Xcf =
    Eigen::Matrix<std::complex<float>, 1, Eigen::Dynamic, Eigen::RowMajor>;

MatrixXYcf fft2_copy(FloatGrayscale img) {
  Eigen::FFT<float> fft;
  MatrixXYcf out = MatrixXYcf::Zero(img.rows(), img.cols());

  for (int i = 0; i < img.rows(); ++i) {
    Matrix1Xcf temp(img.cols());
    fft.fwd(temp, img.row(i));
    out.row(i) = temp;
  }

  for (int j = 0; j < img.cols(); ++j) {
    Matrix1Xcf temp(img.rows());
    fft.fwd(temp, out.col(j));
    out.col(j) = temp;
  }

  return out;
}

template <typename Derived> void fftshift2(Eigen::MatrixBase<Derived> &m) {
  if (m.rows() % 2 != 0 || m.cols() % 2 != 0) {
    std::cerr << "TODO: Handle odd sizes in fftshift2" << std::endl;
    std::abort();
  }

  m.topRows(m.rows() / 2).swap(m.bottomRows(m.rows() / 2));
  m.leftCols(m.cols() / 2).swap(m.rightCols(m.cols() / 2));
}

struct State {
  bool mulWindow = false;
  float coef_a = 100.0, coef_b = 10.0;

  bool operator==(const State &) const = default;
};

struct App {
  SafeGlfwCtx &glfwCtx;
  SafeGlfwWindow &glfwWindow;
  SafeGlew &glew;
  SafeImGui &imguiContext;
  SafeGlTexture tex0, texAbs, texRe, texIm;

  State prev = {false, std::numeric_limits<float>::quiet_NaN(),
                std::numeric_limits<float>::quiet_NaN()};
  State current;

  void frame();
};

void App::frame() {

  GlfwFrame glfwFrame(glfwWindow.window());
  ImGuiGlfwFrame imguiFrame;

  ImGui::SetNextWindowPos({10.0, 10.0}, ImGuiCond_Once);
  if (ImGui::Begin("hello")) {
    ImGui::Checkbox("Multiply by e^{-\\frac{1}{...}(xx^2 + yy^2)}",
                    &current.mulWindow);

    ImGui::SliderFloat("a", &current.coef_a, -196.0, 196.0);
    ImGui::SliderFloat("b", &current.coef_b, -196.0, 196.0);

    if (!(current == prev)) {
      const auto cols = 256, rows = 256;
      const auto uu =
          VectorXf::LinSpaced(cols, -1.0, 1.0).colwise().replicate(rows);
      const auto vv = VectorXf::LinSpaced(rows, -1.0, 1.0)
                          .transpose()
                          .rowwise()
                          .replicate(cols);
      const auto window =
          (-(uu.array().pow(2.0) + vv.array().pow(2.0)) / std::sqrt(0.5)).exp();
      FloatGrayscale src =
          ((current.coef_a * uu.array() + current.coef_b * vv.array()) * .5)
              .sin()
              .matrix();

      if (current.mulWindow) {
        src.array() *= window;
      }

      MatrixXYcf fft2Src = fft2_copy(src);
      fftshift2(fft2Src);

      FloatGrayscale absFft2 = fft2Src.array().abs();
      absFft2 = absFft2 / absFft2.maxCoeff();

      FloatGrayscale reFft2 = fft2Src.real();
      reFft2 = reFft2.array() - reFft2.minCoeff();
      reFft2 = reFft2 / reFft2.maxCoeff();

      FloatGrayscale imFft2 = fft2Src.imag();
      imFft2 = imFft2.array() - imFft2.minCoeff();
      imFft2 = imFft2 / imFft2.maxCoeff();

      tex0.reallocate(1, src.rows(), src.cols(), f32, GL_LINEAR, GL_NEAREST,
                      src.data());
      texAbs.reallocate(1, absFft2.rows(), absFft2.cols(), f32, GL_LINEAR,
                        GL_NEAREST, absFft2.data());
      texRe.reallocate(1, reFft2.rows(), reFft2.cols(), f32, GL_LINEAR,
                       GL_NEAREST, reFft2.data());
      texIm.reallocate(1, imFft2.rows(), imFft2.cols(), f32, GL_LINEAR,
                       GL_NEAREST, imFft2.data());
      prev = current;
    }

    if (ImPlot::BeginPlot("srcImage", {480, 480})) {
      ImPlot::PlotImage("src", tex0.textureVoidStar(), {0.0, 0.0}, {1.0, 1.0});
      ImPlot::EndPlot();
    }
    ImGui::SameLine();
    if (ImPlot::BeginPlot("abs(fft2(src)) plot", {480, 480})) {
      ImPlot::PlotImage("abs(fft2(src))", texAbs.textureVoidStar(), {0.0, 0.0},
                        {1.0, 1.0});
      ImPlot::EndPlot();
    }

    if (ImPlot::BeginPlot("Re(fft2(src)) plot", {480, 480})) {
      ImPlot::PlotImage("Re(fft2(src))", texRe.textureVoidStar(), {0.0, 0.0},
                        {1.0, 1.0});
      ImPlot::EndPlot();
    }
    ImGui::SameLine();
    if (ImPlot::BeginPlot("Im(fft2(src)) plot", {480, 480})) {
      ImPlot::PlotImage("Im(fft2(src))", texIm.textureVoidStar(), {0.0, 0.0},
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
