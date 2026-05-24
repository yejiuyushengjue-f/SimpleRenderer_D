# SimpleRenderer — Software Rasterizer

Software renderer with Win32/OpenGL window, ImGui controls, OBJ loading, texture mapping, and Blinn-Phong lighting.

## Build

```bash
# Visual Studio 2019 (Windows)
mkdir build && cd build
cmake -G "Visual Studio 16 2019" -A x64 ..
cmake --build . --config Debug
```

Or open `build/SimpleRenderer.sln` in Visual Studio.

Dependencies (auto-downloaded by CMake): GLFW 3.3.9, Dear ImGui v1.91.0.

## Project Structure

```
src/
├── main.cpp              # Entry point, scene setup, ImGui light-control panel
├── math.hpp              # Vec2, Vec3, Vec4, Mat4
├── model.hpp/cpp         # Mesh struct, OBJ loader, procedural geometry (cube/plane/sphere)
├── texture.hpp/cpp       # Texture class: bilinear sampling, checkerboard, gradient, PPM load
├── renderer.hpp/cpp      # Full rasterization pipeline + Blinn-Phong shading
└── platform/
    └── glwindow.hpp/cpp  # GLFW window + OpenGL 3.3 context, ImGui init, framebuffer display
```

## Implemented Features

- **Math**: Vec2/3/4, Mat4 (translate/rotate/scale/perspective/lookAt)
- **Model**: OBJ parser (v/vt/vn, fan triangulation, auto-normals), cube, plane, sphere
- **Rasterizer**: edge-function triangle traversal, barycentric coordinates, backface culling, depth buffer (NDC z), perspective-correct interpolation
- **Shading**: Blinn-Phong (ambient + diffuse + specular with half-vector), per-face flat shading, per-vertex smooth shading
- **Texture**: bilinear/nearest sampling, checkerboard/gradient generators, PPM P6 loader
- **Window**: GLFW + OpenGL 3.3, fullscreen quad displays software framebuffer as texture
- **ImGui**: real-time light controls (direction, color, ambient/diffuse/specular, shininess, background)
- **Output**: PPM P6 binary, window display

## Not Yet Implemented

- Shadow mapping / shadows
- Normal mapping
- Anti-aliasing (MSAA / SSAA)
- Alpha blending / transparency
- Near/far plane clipping
- Multi-light / point lights
- Wireframe mode
- Texture file loading from image formats (PNG, JPG)
- Animation / real-time scene updates
