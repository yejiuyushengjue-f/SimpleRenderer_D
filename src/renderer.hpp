#pragma once
#include "math.hpp"
#include "model.hpp"
#include "texture.hpp"
#include <vector>
#include <string>

class Renderer {
public:
    Renderer(int width, int height);

    void clear(const Vec3& color = {0.1f, 0.1f, 0.15f});

    void setViewMatrix(const Mat4& v)    { view = v; }
    void setModelMatrix(const Mat4& m)   { model = m; }
    void setProjectionMatrix(const Mat4& p) { proj = p; }
    void setLightDir(const Vec3& dir)    { lightDir = dir.normalized(); }
    void setLightColor(const Vec3& c)    { lightColor = c; }
    void setCameraPos(const Vec3& pos)   { cameraPos = pos; }
    void setAmbient(float a)             { ambient = a; }
    void setDiffuse(float d)             { diffuse = d; }

    // Material / texture
    void setTexture(const Texture* tex)  { texture = tex; }
    void setSpecular(const Vec3& c, float s) { specColor = c; shininess = s; }

    // Flat-shaded draw
    void drawMesh(const Mesh& mesh);
    // Smooth-shaded draw (perspective-correct normals + UVs)
    void drawMeshSmooth(const Mesh& mesh);

    void savePPM(const std::string& path);

    // Access framebuffer for external display
    const std::vector<Vec3>& getFramebuffer() const { return framebuffer; }

private:
    int  w, h;
    std::vector<Vec3>   framebuffer;
    std::vector<float>  depthbuffer;

    Mat4  view, model, proj;
    Vec3  lightDir   = {0.5f, -1.0f, -0.3f};
    Vec3  lightColor = {1.0f, 1.0f, 1.0f};
    Vec3  cameraPos  = {0, 0, 5};
    float ambient    = 0.15f;
    float diffuse    = 0.85f;
    Vec3  specColor = {1, 1, 1};
    float shininess = 32.0f;
    const Texture* texture = nullptr;

    // Per-vertex data passed through the pipeline
    struct PSIn {
        Vec3 ndc;         // NDC position
        Vec4 clip;        // clip-space (w for perspective correction)
        Vec3 worldPos;    // world-space position (for view direction)
        Vec3 worldNormal; // world-space normal
        Vec2 uv;          // texture coordinate
    };

    static float edge(float ax, float ay, float bx, float by, float cx, float cy) {
        return (cx - ax) * (by - ay) - (cy - ay) * (bx - ax);
    }

    Vec3 shade(const Vec3& worldPos, const Vec3& normal, const Vec2& uv) const;
    void rasterTriangle(const PSIn& v0, const PSIn& v1, const PSIn& v2);
};
