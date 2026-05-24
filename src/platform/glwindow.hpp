#pragma once
#include "math.hpp"
#include <vector>
#include <string>

// Opaque pointer to GLFWwindow
struct GLFWwindow;

class GLWindow {
public:
    GLWindow(int width, int height, const char* title);
    ~GLWindow();

    bool closed() const;
    void beginFrame();                     // clear + new ImGui frame
    void present(const Vec3* pixels);      // upload framebuffer → GL texture → draw
    void endFrame();                       // render ImGui → swap buffers → poll events

    int  width()  const { return w; }
    int  height() const { return h; }

    // For ImGui
    GLFWwindow* handle() const { return window; }

private:
    int w, h;
    GLFWwindow* window = nullptr;
    unsigned    fbTexture = 0; // OpenGL texture for framebuffer
    bool        initialized = false;

    void initGL();
    void loadGLFunctions();
};

// Call once at startup
bool GLWindow_InitImGui(GLFWwindow* window);
void GLWindow_ShutdownImGui();
