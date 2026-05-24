cmake_minimum_required(VERSION 3.14)
project(SimpleRenderer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/W4 /utf-8)
else()
    add_compile_options(-Wall -Wextra)
endif()

include(FetchContent)

# ------------------------------------------------------------------
# GLFW — use URL (HTTPS) instead of GIT to avoid protocol issues
# ------------------------------------------------------------------
FetchContent_Declare(glfw
    URL  https://github.com/glfw/glfw/archive/refs/tags/3.3.9.zip
)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

# ------------------------------------------------------------------
# Dear ImGui — URL download + manual library target
# ------------------------------------------------------------------
FetchContent_Declare(imgui
    URL  https://github.com/ocornut/imgui/archive/refs/tags/v1.91.0.zip
)
FetchContent_Populate(imgui)

add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR})
target_link_libraries(imgui PUBLIC glfw)

# ------------------------------------------------------------------
# Renderer executable
# ------------------------------------------------------------------
add_executable(renderer
    src/main.cpp
    src/model.cpp
    src/renderer.cpp
    src/texture.cpp
    src/platform/glwindow.cpp
)

target_include_directories(renderer PRIVATE src)
target_link_libraries(renderer PRIVATE imgui glfw opengl32)
