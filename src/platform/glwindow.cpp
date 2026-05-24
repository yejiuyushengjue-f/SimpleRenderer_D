#include "glwindow.hpp"

// GLFW
#include <GLFW/glfw3.h>

// Dear ImGui
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include <cstdio>
#include <cstdint>

// ============================================================================
// Windows SDK provides <GL/gl.h> with GL 1.1 types + functions.
// We need extension types and constants that are NOT in gl.h.
// ============================================================================
#include <GL/gl.h>   // GLuint, GLint, GLfloat, GLenum, GLvoid, etc.

// Extension types (not in Windows SDK gl.h)
typedef ptrdiff_t   GLsizeiptr;
typedef ptrdiff_t   GLintptr;
typedef char        GLchar;

// Extension constants
#define GL_COMPILE_STATUS       0x8B81
#define GL_VERTEX_SHADER        0x8B31
#define GL_FRAGMENT_SHADER      0x8B30
#define GL_ARRAY_BUFFER         0x8892
#define GL_ELEMENT_ARRAY_BUFFER 0x8893
#define GL_STATIC_DRAW          0x88E4
#define GL_CLAMP_TO_EDGE        0x812F
#define GL_TEXTURE0             0x84C0
#define GL_RGB8                 0x8051

// ============================================================================
// Extension function pointer types + globals
// ============================================================================
#define DECL_PFN(ret, name, params) \
    typedef ret (APIENTRY *PFN_##name) params; \
    static PFN_##name name = nullptr;

// VAOs
DECL_PFN(void,   glGenVertexArrays,    (GLsizei n, GLuint* arrays))
DECL_PFN(void,   glBindVertexArray,    (GLuint array))
DECL_PFN(void,   glDeleteVertexArrays, (GLsizei n, const GLuint* arrays))
// VBOs
DECL_PFN(void,   glGenBuffers,     (GLsizei n, GLuint* buffers))
DECL_PFN(void,   glBindBuffer,     (GLenum target, GLuint buffer))
DECL_PFN(void,   glBufferData,     (GLenum target, GLsizeiptr size, const void* data, GLenum usage))
DECL_PFN(void,   glDeleteBuffers,  (GLsizei n, const GLuint* buffers))
// Shaders
DECL_PFN(GLuint, glCreateShader,      (GLenum type))
DECL_PFN(void,   glShaderSource,      (GLuint s, GLsizei count, const GLchar** str, const GLint* len))
DECL_PFN(void,   glCompileShader,     (GLuint s))
DECL_PFN(void,   glGetShaderiv,       (GLuint s, GLenum pname, GLint* params))
DECL_PFN(void,   glGetShaderInfoLog,  (GLuint s, GLsizei bufSize, GLsizei* len, GLchar* log))
DECL_PFN(void,   glDeleteShader,      (GLuint s))
DECL_PFN(GLuint, glCreateProgram,     (void))
DECL_PFN(void,   glAttachShader,      (GLuint program, GLuint shader))
DECL_PFN(void,   glLinkProgram,       (GLuint program))
DECL_PFN(void,   glGetProgramiv,      (GLuint program, GLenum pname, GLint* params))
DECL_PFN(void,   glGetProgramInfoLog, (GLuint program, GLsizei bufSize, GLsizei* len, GLchar* log))
DECL_PFN(void,   glUseProgram,        (GLuint program))
DECL_PFN(void,   glDeleteProgram,     (GLuint program))
// Textures (extension versions — GL 1.1 versions are in gl.h but use different signatures)
DECL_PFN(void,   glActiveTexture,     (GLenum texture))
// Vertex attribs
DECL_PFN(void,   glVertexAttribPointer,     (GLuint idx, GLint size, GLenum type, GLboolean nrm, GLsizei stride, const void* ptr))
DECL_PFN(void,   glEnableVertexAttribArray,  (GLuint idx))
DECL_PFN(void,   glDisableVertexAttribArray, (GLuint idx))
// Uniforms
DECL_PFN(GLint,  glGetUniformLocation, (GLuint program, const GLchar* name))
DECL_PFN(void,   glUniform1i,          (GLint location, GLint v))
DECL_PFN(void,   glUniform1f,          (GLint location, GLfloat v))
DECL_PFN(void,   glUniform3f,          (GLint location, GLfloat a, GLfloat b, GLfloat c))
DECL_PFN(void,   glUniformMatrix4fv,   (GLint location, GLsizei count, GLboolean transpose, const GLfloat* value))

#undef DECL_PFN

// ============================================================================
// Shaders — fullscreen quad
// ============================================================================
static const char* kVertShader = R"(#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aUV;
out vec2 vUV;
void main() {
    gl_Position = vec4(aPos, 0.0, 1.0);
    vUV = aUV;
}
)";

static const char* kFragShader = R"(#version 330 core
in vec2 vUV;
out vec4 FragColor;
uniform sampler2D uTexture;
void main() {
    FragColor = texture(uTexture, vUV);
}
)";

static GLuint gProgram = 0;
static GLuint gVAO = 0, gVBO = 0, gEBO = 0;

// ---------------------------------------------------------------------------
void GLWindow::loadGLFunctions() {
#define LOAD(name) name = (PFN_##name)glfwGetProcAddress(#name)
    LOAD(glGenVertexArrays);
    LOAD(glBindVertexArray);
    LOAD(glDeleteVertexArrays);
    LOAD(glGenBuffers);
    LOAD(glBindBuffer);
    LOAD(glBufferData);
    LOAD(glDeleteBuffers);
    LOAD(glCreateShader);
    LOAD(glShaderSource);
    LOAD(glCompileShader);
    LOAD(glGetShaderiv);
    LOAD(glGetShaderInfoLog);
    LOAD(glDeleteShader);
    LOAD(glCreateProgram);
    LOAD(glAttachShader);
    LOAD(glLinkProgram);
    LOAD(glGetProgramiv);
    LOAD(glGetProgramInfoLog);
    LOAD(glUseProgram);
    LOAD(glDeleteProgram);
    LOAD(glActiveTexture);
    LOAD(glVertexAttribPointer);
    LOAD(glEnableVertexAttribArray);
    LOAD(glDisableVertexAttribArray);
    LOAD(glGetUniformLocation);
    LOAD(glUniform1i);
    LOAD(glUniform1f);
    LOAD(glUniform3f);
    LOAD(glUniformMatrix4fv);
#undef LOAD
}

// ---------------------------------------------------------------------------
static GLuint compileShader(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, NULL);
    glCompileShader(s);
    GLint ok;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetShaderInfoLog(s, 512, NULL, log);
        fprintf(stderr, "Shader error: %s\n", log);
    }
    return s;
}

void GLWindow::initGL() {
    loadGLFunctions();

    GLuint vs = compileShader(GL_VERTEX_SHADER,   kVertShader);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, kFragShader);
    gProgram = glCreateProgram();
    glAttachShader(gProgram, vs);
    glAttachShader(gProgram, fs);
    glLinkProgram(gProgram);
    glDeleteShader(vs);
    glDeleteShader(fs);

    // Fullscreen quad
    float quad[] = {
        -1,-1, 0,0,
         1,-1, 1,0,
         1, 1, 1,1,
        -1, 1, 0,1,
    };
    unsigned idx[] = {0, 1, 2, 0, 2, 3};

    glGenVertexArrays(1, &gVAO);
    glBindVertexArray(gVAO);

    glGenBuffers(1, &gVBO);
    glBindBuffer(GL_ARRAY_BUFFER, gVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);

    glGenBuffers(1, &gEBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)(2*sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    // Framebuffer texture
    glGenTextures(1, &fbTexture);
    glBindTexture(GL_TEXTURE_2D, fbTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glBindTexture(GL_TEXTURE_2D, 0);

    initialized = true;
}

// ============================================================================
// GLWindow
// ============================================================================
GLWindow::GLWindow(int width, int height, const char* title)
    : w(width), h(height)
{
    if (!glfwInit()) { fprintf(stderr, "GLFW init failed\n"); return; }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(w, h, title, NULL, NULL);
    if (!window) { fprintf(stderr, "GLFW window failed\n"); glfwTerminate(); return; }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    initGL();
    GLWindow_InitImGui(window);
}

GLWindow::~GLWindow() {
    GLWindow_ShutdownImGui();
    if (fbTexture)   glDeleteTextures(1, &fbTexture);
    if (gVAO)        glDeleteVertexArrays(1, &gVAO);
    if (gVBO)        glDeleteBuffers(1, &gVBO);
    if (gEBO)        glDeleteBuffers(1, &gEBO);
    if (gProgram)    glDeleteProgram(gProgram);
    if (window)      glfwDestroyWindow(window);
    glfwTerminate();
}

bool GLWindow::closed() const {
    return !window || glfwWindowShouldClose(window);
}

void GLWindow::beginFrame() {
    if (!initialized) return;
    glfwPollEvents();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

void GLWindow::present(const Vec3* pixels) {
    if (!initialized) return;

    std::vector<uint8_t> rgb(w * h * 3);
    for (int i = 0; i < w * h; i++) {
        auto sat = [](float v) -> uint8_t {
            v = v < 0 ? 0 : (v > 1 ? 1 : v);
            return (uint8_t)(v * 255.0f + 0.5f);
        };
        rgb[i*3 + 0] = sat(pixels[i].x);
        rgb[i*3 + 1] = sat(pixels[i].y);
        rgb[i*3 + 2] = sat(pixels[i].z);
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, fbTexture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, rgb.data());

    glUseProgram(gProgram);
    glUniform1i(glGetUniformLocation(gProgram, "uTexture"), 0);

    glBindVertexArray(gVAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void GLWindow::endFrame() {
    if (!initialized) return;
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(window);
}

// ---------------------------------------------------------------------------
bool GLWindow_InitImGui(GLFWwindow* window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");
    printf("ImGui initialized (GLFW + OpenGL 3.3)\n");
    return true;
}

void GLWindow_ShutdownImGui() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}
