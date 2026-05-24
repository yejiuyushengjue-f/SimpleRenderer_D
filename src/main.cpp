#include "renderer.hpp"
#include "platform/glwindow.hpp"
#include <imgui.h>
#include <cstdio>

// ---------------------------------------------------------------------------
// Scene parameters (adjustable via ImGui)
// ---------------------------------------------------------------------------
struct SceneParams {
    // Light direction
    float lx =  0.6f, ly = -1.0f, lz = -0.4f;
    // Light color
    float lr = 1.0f, lg = 1.0f, lb = 1.0f;
    // Light strengths
    float ambient  = 0.15f;
    float diffuse  = 0.85f;
    float specular = 0.5f;
    // Specular
    float specR = 1.0f, specG = 1.0f, specB = 1.0f;
    float shininess = 64.0f;
    // Background
    float bgR = 0.15f, bgG = 0.15f, bgB = 0.2f;
    bool  autoRender = true;
};

// ---------------------------------------------------------------------------
// ImGui control panel — returns true if any param changed
// ---------------------------------------------------------------------------
bool showControlPanel(SceneParams& p) {
    ImGui::Begin("Light Controls");

    bool changed = false;

    // --- Light direction ---
    changed |= ImGui::DragFloat3("Light Dir", &p.lx, 0.05f, -2.0f, 2.0f);
    ImGui::Text("(X=right, Y=up, Z=toward camera)");

    ImGui::Separator();

    // --- Light color ---
    changed |= ImGui::ColorEdit3("Light Color", &p.lr,
        ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);

    ImGui::Separator();

    // --- Strengths ---
    changed |= ImGui::SliderFloat("Ambient",  &p.ambient,  0.0f, 1.0f);
    changed |= ImGui::SliderFloat("Diffuse",  &p.diffuse,  0.0f, 1.0f);

    ImGui::Separator();

    // --- Specular ---
    ImGui::Text("Specular Highlight");
    changed |= ImGui::ColorEdit3("Spec Color", &p.specR,
        ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
    changed |= ImGui::SliderFloat("Shininess", &p.shininess, 1.0f, 256.0f, "%.0f");

    ImGui::Separator();

    // --- Background ---
    changed |= ImGui::ColorEdit3("Background", &p.bgR,
        ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);

    ImGui::Spacing();
    ImGui::Checkbox("Auto Re-render", &p.autoRender);
    if (!p.autoRender) {
        if (ImGui::Button("Re-render")) changed = true;
    }

    static float avgMs = 0;
    ImGui::Text("(controls update in real-time)");

    ImGui::End();
    return changed;
}

// ---------------------------------------------------------------------------
// Render scene with current params
// ---------------------------------------------------------------------------
void renderScene(Renderer& renderer, const SceneParams& p, Texture& checkerTex, int argc, char** argv) {
    renderer.clear({p.bgR, p.bgG, p.bgB});

    renderer.setLightDir({p.lx, p.ly, p.lz});
    renderer.setLightColor({p.lr, p.lg, p.lb});
    renderer.setAmbient(p.ambient);
    renderer.setDiffuse(p.diffuse);

    // Ground plane
    renderer.setModelMatrix(Mat4::translate(0, -0.55f, 0));
    renderer.setTexture(&checkerTex);
    renderer.setSpecular({0.1f, 0.1f, 0.1f}, 4.0f);
    renderer.drawMesh(createPlane(8.0f, 4.0f));

    // Center cube
    renderer.setTexture(nullptr);
    renderer.setModelMatrix(
        Mat4::translate(0, 0, 0) *
        Mat4::rotateY(0.5f) *
        Mat4::scale(0.7f, 0.7f, 0.7f));
    renderer.setSpecular({p.specR, p.specG, p.specB}, p.shininess);
    renderer.drawMesh(createCube(1.0f));

    // Left sphere
    renderer.setModelMatrix(Mat4::translate(-0.8f, 0.65f, 0.4f));
    renderer.setSpecular({p.specR, p.specG, p.specB}, p.shininess);
    renderer.drawMeshSmooth(createSphere(0.35f, 32));

    // Right sphere
    renderer.setModelMatrix(Mat4::translate(0.9f, 0.3f, -0.3f));
    renderer.setSpecular({p.specR, p.specG, p.specB}, p.shininess);
    renderer.drawMeshSmooth(createSphere(0.45f, 32));

    // Small cube in front
    renderer.setModelMatrix(
        Mat4::translate(0.2f, -0.2f, 1.2f) *
        Mat4::rotateY(-0.3f));
    renderer.setSpecular({p.specR, p.specG, p.specB}, p.shininess);
    renderer.drawMesh(createCube(0.4f));

    // External OBJ
    if (argc > 1) {
        static Mesh ext = loadOBJ(argv[1]);
        if (!ext.indices.empty()) {
            renderer.setModelMatrix(
                Mat4::translate(0, 0.15f, 0) *
                Mat4::scale(0.8f, 0.8f, 0.8f));
            renderer.setTexture(nullptr);
            renderer.drawMeshSmooth(ext);
        }
    }
}

// ---------------------------------------------------------------------------
int main(int argc, char** argv) {
    const int W = 1600, H = 1200;

    // Camera
    Vec3 eye    = {2.5f, 2.2f, 3.5f};
    Vec3 center = {0, 0.3f, 0};

    Mat4 viewMat = Mat4::lookAt(eye, center, {0, 1, 0});
    Mat4 projMat = Mat4::perspective(3.14159265f * 0.35f, (float)W / H, 0.1f, 100.0f);

    // Renderer
    Renderer renderer(W, H);
    renderer.setViewMatrix(viewMat);
    renderer.setProjectionMatrix(projMat);
    renderer.setCameraPos(eye);

    // Texture cache
    Texture checkerTex = Texture::checkerboard(512, 8);

    // Scene parameters
    SceneParams params;

    // Window
    GLWindow window(W, H, "Simple Renderer — ImGui Light Controls");

    bool firstFrame = true;
    bool needsRender = true;

    while (!window.closed()) {
        window.beginFrame();

        bool changed = showControlPanel(params);
        if (changed && params.autoRender) needsRender = true;
        if (changed) needsRender = true;

        if (needsRender || firstFrame) {
            renderScene(renderer, params, checkerTex, argc, argv);
            needsRender = false;
            firstFrame = false;
        }

        window.present(renderer.getFramebuffer().data());
        window.endFrame();
    }

    renderer.savePPM("output.ppm");
    return 0;
}
