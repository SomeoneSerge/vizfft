#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>

#define GL_GLEXT_PROTOTYPES
#define EGL_EGLEXT_PROTOTYPES

// #define GLFW_INCLUDE_ES3
// #include <GLES3/gl3.h>
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
  bool mulWindow = true;
  bool logAbs = true;
  float windowStd = .125;
  float coef_a = 25.0 / std::numbers::pi;
  float coef_b = 10.0 / std::numbers::pi;

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

constexpr auto nyquist = 128;

struct App {
  SafeSDLSession &initSDL;
  SafeSDLWindow &sdlWindow;
  SafeImGui &imguiContext;
  SafeGlTexture tex0, texAbs, texArg;
  Heatmap heatAbs, heatArg;
  ImEditVec2 editCoeffs;

  bool shouldClose = false;

  State prev = {.mulWindow = false,
                .coef_a = std::numeric_limits<float>::quiet_NaN(),
                .coef_b = std::numeric_limits<float>::quiet_NaN()};
  State current;

  void frame();
};

void App::frame() {

  SDLFrame sdlFrame(sdlWindow.window());
  ImGuiSDLFrame imguiFrame(sdlWindow);

  shouldClose |= sdlFrame.shouldClose();

  ImVec2 srcWindowSize = {500.0f, 640.0f};
  ImGui::SetNextWindowSize(srcWindowSize, ImGuiCond_Once);
  ImGui::SetNextWindowPos(ImVec2(10.0, 10.0), ImGuiCond_Once);
  if (ImBeginWindow wnd("Input image"); wnd.visible) {
    if (!(current == prev)) {
      const auto cols = nyquist * 2, rows = nyquist * 2;
      const auto uu =
          VectorXf::LinSpaced(cols, -1.0, 1.0).colwise().replicate(rows);
      const auto vv = VectorXf::LinSpaced(rows, -1.0, 1.0)
                          .transpose()
                          .rowwise()
                          .replicate(cols);
      const auto window = (-(uu.array().pow(2.0) + vv.array().pow(2.0)) /
                           std::sqrt(current.windowStd))
                              .exp();
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

      if (current.logAbs) {
        heatAbs.load(fft2Src.array().abs().log().abs().matrix());
      } else {
        heatAbs.load(fft2Src.cwiseAbs());
      };
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

    if (ImBeginPlot plt("srcImage", {480, 480}); plt.visible) {
      ImPlot::PlotImage("src", tex0.textureVoidStar(), {0.0, 0.0}, {1.0, 1.0});
    }

    ImGui::Checkbox("Log scale absolute value", &current.logAbs);

    ImGui::Text("Frequencies along x and y");
    editCoeffs.show(current.coef_a, current.coef_b);

    ImGui::Checkbox("Apply Gaussian window", &current.mulWindow);
    if (current.mulWindow) {
      ImGui::SliderFloat("std", &current.windowStd, 0.005, 1.0, nullptr,
                         ImGuiSliderFlags_Logarithmic);
    }
  }

  ImGui::SetNextWindowPos(ImVec2(srcWindowSize.x + 10 + 10, 10),
                          ImGuiCond_Once);
  if (ImBeginWindow wnd("Magnitude-Phase"); wnd.visible) {
    const char *absPlotTitle =
        current.logAbs ? "log(abs(fft2(src)))" : "abs(fft2(src))";
    if (struct {
          ImPushId pltId;
          ImBeginPlot plt;
        } s{.pltId = ImPushId("absPlot"),
            .plt = ImBeginPlot(absPlotTitle, {480, 480})};
        s.plt.visible) {
      ImPlot::PlotImage(absPlotTitle, texAbs.textureVoidStar(), {0.0, 0.0},
                        {1.0, 1.0});
    }
    ImGui::SameLine();
    if (ImBeginPlot plt("arg(fft2(src)) plot", {480, 480}); plt.visible) {
      ImPlot::PlotImage("arg(fft2(src))", texArg.textureVoidStar(), {0.0, 0.0},
                        {1.0, 1.0});
    }
  }
}

std::function<void()> frame_global;
void call_frame_global() { frame_global(); }

int main(int argc, char *argv[]) {
  SafeSDLSession sdlSession;
  SafeSDLWindow sdlWindow;
  sdlWindow.makeContextCurrent();

  // It would seem it's not neccessary to glewInit() after all o_0
  // SafeGlew glewCtx;

  SafeImGui imguiContext(sdlWindow.window(), sdlWindow.context());

  std::cout << "glGetString(GL_VERSION): " << glGetString(GL_VERSION)
            << std::endl;

  App app = {.initSDL = sdlSession,
             .sdlWindow = sdlWindow,
             .imguiContext = imguiContext,
             .editCoeffs = ImEditVec2("coeffs", ImVec2(-nyquist, nyquist),
                                      ImVec2(-nyquist, nyquist))};

#ifdef __EMSCRIPTEN__
  frame_global = [&]() { app.frame(); };
  emscripten_set_main_loop(call_frame_global, 0, true);
#else
  while (!app.shouldClose) {
    app.frame();
  }
#endif

  return 0;
}
