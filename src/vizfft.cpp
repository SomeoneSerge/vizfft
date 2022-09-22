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
#include <numbers>
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
  float coef_a = 100.0 / std::numbers::pi, coef_b = 10.0 / std::numbers::pi;

  bool operator==(const State &) const = default;
};

struct Heatmap {
  /* Linearly normalized heatmap */

  float minCoeff, maxCoeff;
  float minDisplay, maxDisplay;
  FloatGrayscale normalizedMap;
  FloatGrayscale tex;

  Heatmap()
      : minCoeff(std::numeric_limits<float>::quiet_NaN()),
        maxCoeff(std::numeric_limits<float>::quiet_NaN()),
        minDisplay(std::numeric_limits<float>::quiet_NaN()),
        maxDisplay(std::numeric_limits<float>::quiet_NaN()) {}

  Heatmap(const FloatGrayscale &src) { load(src); }

  template <typename Derived> void load(const Eigen::MatrixBase<Derived> &src) {
    minCoeff = src.minCoeff();
    maxCoeff = src.maxCoeff();
    const auto h = std::max(maxCoeff - minCoeff, 1e-9f);
    const auto mi = .5 * minCoeff + .5 * maxCoeff;
    normalizedMap = (src.array() - mi) / h;
  }

  void computeTex(const float newMin, const float newMax) {
    minDisplay = newMin;
    maxDisplay = newMax;
    const auto oldMi = .5 * minCoeff + .5 * maxCoeff;
    const auto oldH = std::max(maxCoeff - minCoeff, 1e-9f);
    const auto newH = std::max(maxDisplay - minDisplay, 1e-9f);
    tex = normalizedMap.array() * (oldH / newH) + (oldMi - minDisplay) / newH;
    tex = tex.array().min(1.0).max(0.0);
  }
};

struct App {
  SafeGlfwCtx &glfwCtx;
  SafeGlfwWindow &glfwWindow;
  SafeGlew &glew;
  SafeImGui &imguiContext;
  SafeGlTexture tex0, texAbs, texArg;
  Heatmap heatAbs, heatArg;

  State prev = {false, std::numeric_limits<float>::quiet_NaN(),
                std::numeric_limits<float>::quiet_NaN()};
  State current;

  void frame();
};

constexpr auto nyquist = 128;

void App::frame() {

  GlfwFrame glfwFrame(glfwWindow.window());
  ImGuiGlfwFrame imguiFrame;

  ImVec2 srcWindowSize = {500.0f, 600.0f};
  ImGui::SetNextWindowSize(srcWindowSize, ImGuiCond_Once);
  ImGui::SetNextWindowPos({10.0, 10.0}, ImGuiCond_Once);
  if (ImGui::Begin("Input image")) {
    ImGui::Checkbox("Multiply by e^{-\\frac{1}{...}(xx^2 + yy^2)}",
                    &current.mulWindow);

    ImGui::SliderFloat("a", &current.coef_a, -nyquist, nyquist);
    ImGui::SliderFloat("b", &current.coef_b, -nyquist, nyquist);

    if (!(current == prev)) {
      const auto cols = nyquist * 2, rows = nyquist * 2;
      const auto uu =
          VectorXf::LinSpaced(cols, -1.0, 1.0).colwise().replicate(rows);
      const auto vv = VectorXf::LinSpaced(rows, -1.0, 1.0)
                          .transpose()
                          .rowwise()
                          .replicate(cols);
      const auto window =
          (-(uu.array().pow(2.0) + vv.array().pow(2.0)) / std::sqrt(0.5)).exp();
      FloatGrayscale src =
          ((current.coef_a * uu.array() + current.coef_b * vv.array()) *
           std::numbers::pi)
              .sin()
              .matrix();

      if (current.mulWindow) {
        src.array() *= window;
      }

      MatrixXYcf fft2Src = fft2_copy(src);
      fftshift2(fft2Src);

      heatAbs.load(fft2Src.cwiseAbs());
      heatArg.load(fft2Src.cwiseArg());

      const auto lo = heatAbs.minCoeff;
      const auto hi = heatAbs.maxCoeff;

      heatAbs.computeTex(0, hi);
      heatArg.computeTex(-std::numbers::pi, std::numbers::pi);

      tex0.reallocate(1, src.rows(), src.cols(), f32, GL_LINEAR, GL_NEAREST,
                      src.data());
      texAbs.reallocate(1, src.rows(), src.cols(), f32, GL_LINEAR, GL_NEAREST,
                        heatAbs.tex.data());
      texArg.reallocate(1, src.rows(), src.cols(), f32, GL_LINEAR, GL_NEAREST,
                        heatArg.tex.data());
      prev = current;
    }

    if (ImPlot::BeginPlot("srcImage", {480, 480})) {
      ImPlot::PlotImage("src", tex0.textureVoidStar(), {0.0, 0.0}, {1.0, 1.0});
      ImPlot::EndPlot();
    }
  }
  ImGui::End();

  ImGui::SetNextWindowPos({srcWindowSize.x + 10 + 10, 10}, ImGuiCond_Once);
  if (ImGui::Begin("Magnitude-Phase")) {
    if (ImPlot::BeginPlot("abs(fft2(src)) plot", {480, 480})) {
      ImPlot::PlotImage("abs(fft2(src))", texAbs.textureVoidStar(), {0.0, 0.0},
                        {1.0, 1.0});
      ImPlot::EndPlot();
    }
    ImGui::SameLine();
    if (ImPlot::BeginPlot("arg(fft2(src)) plot", {480, 480})) {
      ImPlot::PlotImage("arg(fft2(src))", texArg.textureVoidStar(), {0.0, 0.0},
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
