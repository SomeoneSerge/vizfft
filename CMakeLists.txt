cmake_minimum_required(VERSION 3.22)

project(vizfft)

option(BUILD_EMSCRIPTEN "Build the emscripten target" ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH)

add_library(
  imgui STATIC
  external/imgui/imgui.cpp
  external/imgui/imgui_demo.cpp
  external/imgui/imgui_draw.cpp
  external/imgui/imgui_tables.cpp
  external/imgui/imgui_widgets.cpp
  external/imgui/backends/imgui_impl_glfw.cpp
  external/imgui/backends/imgui_impl_opengl3.cpp
  external/implot/implot.cpp
  external/implot/implot_demo.cpp
  external/implot/implot_items.cpp
)

add_executable(
  vizfft
  src/vizfft.cpp
)

target_include_directories(
  imgui PUBLIC
  external/imgui
  external/imgui/backends
  external/implot
)

target_include_directories(
  vizfft PUBLIC
  include/
)

find_package(Eigen3 REQUIRED)

if(BUILD_EMSCRIPTEN)
  set_target_properties(vizfft PROPERTIES SUFFIX ".html")

  set_target_properties(
    ${PROJECT_NAME} PROPERTIES LINK_FLAGS
    "-s USE_GLFW=3 -s MIN_WEBGL_VERSION=2 -s MAX_WEBGL_VERSION=2 --shell-file ${PROJECT_SOURCE_DIR}/shell.html"
  )
else()
  find_package(GLEW REQUIRED)
  find_package(glfw3 REQUIRED)
  find_package(OpenGL REQUIRED)

  target_link_libraries(
    vizfft
    
    GLEW
    glfw
    OpenGL::GL
  )
endif()

target_link_libraries(
  vizfft
  
  imgui
  Eigen3::Eigen
)
