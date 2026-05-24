#include "renderer.hpp"
#include <fstream>
#include <algorithm>
#include <cstdio>

Renderer::Renderer(int width, int height)
    : w(width), h(height)
{
    framebuffer.resize(w * h, {0, 0, 0});
    depthbuffer.resize(w * h, 1.0f);
    view  = Mat4::identity();
    model = Mat4::identity();
    proj  = Mat4::identity();
}

void Renderer::clear(const Vec3& color) {
    std::fill(framebuffer.begin(), framebuffer.end(), color);
    std::fill(depthbuffer.begin(), depthbuffer.end(), 1.0f);
}

// ---------------------------------------------------------------------------
// Blinn-Phong + texture fragment shader
// ---------------------------------------------------------------------------
Vec3 Renderer::shade(const Vec3& worldPos, const Vec3& normal, const Vec2& uv) const {
    // Albedo from texture or solid white
    Vec3 albedo = {1, 1, 1};
    if (texture) albedo = texture->sampleBilinear(uv.x, uv.y);

    Vec3 N = normal.normalized();
    Vec3 L = -lightDir;                                      // direction TO light
    Vec3 V = (cameraPos - worldPos).normalized();            // direction TO camera
    Vec3 H = (L + V).normalized();                           // half-vector

    float NdotL = std::max(0.0f, N.dot(L));
    float NdotH = std::max(0.0f, N.dot(H));
    float spec  = std::pow(NdotH, shininess);

    Vec3 ambientTerm  = albedo * lightColor * ambient;
    Vec3 diffuseTerm  = albedo * lightColor * diffuse * NdotL;
    Vec3 specularTerm = specColor * lightColor * spec * (NdotL > 0 ? 1.0f : 0.0f);

    return ambientTerm + diffuseTerm + specularTerm;
}

// ---------------------------------------------------------------------------
// Flat-shaded entry point
// ---------------------------------------------------------------------------
void Renderer::drawMesh(const Mesh& mesh) {
    Mat4 mvp = proj * view * model;

    for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
        int flatNrmIdx = mesh.indices[i]; // all three use same normal

        PSIn v[3];
        for (int k = 0; k < 3; k++) {
            int idx = mesh.indices[i + k];
            Vec4 clip = mvp * Vec4(mesh.positions[idx], 1);
            v[k].ndc     = clip.xyz();
            v[k].clip    = clip;
            v[k].worldPos   = (model * Vec4(mesh.positions[idx], 1.0f)).xyz();
            v[k].worldNormal = model.transformDir(mesh.normals[flatNrmIdx]);
            v[k].uv      = mesh.texcoords.empty() ? Vec2{0, 0} : mesh.texcoords[idx];
        }

        float area = edge(v[0].ndc.x, v[0].ndc.y,
                          v[1].ndc.x, v[1].ndc.y,
                          v[2].ndc.x, v[2].ndc.y);
        if (area <= 0) continue;

        rasterTriangle(v[0], v[1], v[2]);
    }
}

// ---------------------------------------------------------------------------
// Smooth-shaded entry point
// ---------------------------------------------------------------------------
void Renderer::drawMeshSmooth(const Mesh& mesh) {
    Mat4 mvp = proj * view * model;

    for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
        PSIn v[3];
        for (int k = 0; k < 3; k++) {
            int idx = mesh.indices[i + k];
            Vec4 clip = mvp * Vec4(mesh.positions[idx], 1);
            v[k].ndc     = clip.xyz();
            v[k].clip    = clip;
            v[k].worldPos   = (model * Vec4(mesh.positions[idx], 1.0f)).xyz();
            v[k].worldNormal = model.transformDir(mesh.normals[idx]);
            v[k].uv      = mesh.texcoords.empty() ? Vec2{0, 0} : mesh.texcoords[idx];
        }

        float area = edge(v[0].ndc.x, v[0].ndc.y,
                          v[1].ndc.x, v[1].ndc.y,
                          v[2].ndc.x, v[2].ndc.y);
        if (area <= 0) continue;

        rasterTriangle(v[0], v[1], v[2]);
    }
}

// ---------------------------------------------------------------------------
// Triangle rasterization with perspective-correct interpolation
// ---------------------------------------------------------------------------
void Renderer::rasterTriangle(const PSIn& v0, const PSIn& v1, const PSIn& v2) {
    auto toScreen = [&](float ndcX, float ndcY) {
        return Vec3{(ndcX + 1.0f) * 0.5f * w,
                    (1.0f - ndcY) * 0.5f * h, 0};
    };

    Vec3 s0 = toScreen(v0.ndc.x, v0.ndc.y);
    Vec3 s1 = toScreen(v1.ndc.x, v1.ndc.y);
    Vec3 s2 = toScreen(v2.ndc.x, v2.ndc.y);

    int minX = std::max(0,          (int)std::floor(std::min({s0.x, s1.x, s2.x})));
    int minY = std::max(0,          (int)std::floor(std::min({s0.y, s1.y, s2.y})));
    int maxX = std::min(w - 1, (int)std::ceil(std::max({s0.x, s1.x, s2.x})));
    int maxY = std::min(h - 1, (int)std::ceil(std::max({s0.y, s1.y, s2.y})));

    float area = edge(s0.x, s0.y, s1.x, s1.y, s2.x, s2.y);
    if (std::abs(area) < 1e-6f) return;

    float invW0 = 1.0f / v0.clip.w;
    float invW1 = 1.0f / v1.clip.w;
    float invW2 = 1.0f / v2.clip.w;

    for (int y = minY; y <= maxY; y++) {
        for (int x = minX; x <= maxX; x++) {
            float px = (float)x + 0.5f;
            float py = (float)y + 0.5f;

            float w0 = edge(s1.x, s1.y, s2.x, s2.y, px, py) / area;
            float w1 = edge(s2.x, s2.y, s0.x, s0.y, px, py) / area;
            float w2 = edge(s0.x, s0.y, s1.x, s1.y, px, py) / area;

            if (w0 < 0 || w1 < 0 || w2 < 0) continue;

            float z = 1.0f / (w0 * invW0 + w1 * invW1 + w2 * invW2);
            float ndcZ = (v0.ndc.z * w0 * invW0 + v1.ndc.z * w1 * invW1 +
                          v2.ndc.z * w2 * invW2) * z;
            float depth = ndcZ * 0.5f + 0.5f;

            int idx = y * w + x;
            if (depth >= depthbuffer[idx]) continue;
            depthbuffer[idx] = depth;

            // Perspective-correct interpolation helpers
            float bw0 = w0 * invW0, bw1 = w1 * invW1, bw2 = w2 * invW2;

            Vec3 wp = (v0.worldPos * bw0 + v1.worldPos * bw1 + v2.worldPos * bw2) * z;
            Vec3 wn = (v0.worldNormal * bw0 + v1.worldNormal * bw1 + v2.worldNormal * bw2) * z;
            Vec2 uv = (v0.uv * bw0 + v1.uv * bw1 + v2.uv * bw2) * z;

            framebuffer[idx] = shade(wp, wn, uv);
        }
    }
}

// ---------------------------------------------------------------------------
// PPM P6 output
// ---------------------------------------------------------------------------
void Renderer::savePPM(const std::string& path) {
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        fprintf(stderr, "ERROR: cannot write %s\n", path.c_str());
        return;
    }
    file << "P6\n" << w << " " << h << "\n255\n";
    for (const auto& p : framebuffer) {
        auto sat = [](float v) -> unsigned char {
            v = v < 0 ? 0 : (v > 1 ? 1 : v);
            return (unsigned char)(v * 255.0f + 0.5f);
        };
        unsigned char rgb[3] = {sat(p.x), sat(p.y), sat(p.z)};
        file.write((const char*)rgb, 3);
    }
    printf("Saved %s  (%dx%d)\n", path.c_str(), w, h);
}
