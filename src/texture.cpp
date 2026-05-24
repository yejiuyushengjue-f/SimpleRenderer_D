#include "texture.hpp"
#include <fstream>
#include <algorithm>
#include <cstdio>
#include <cstring>

Vec3 Texture::texel(int x, int y) const {
    // Clamp to edge
    x = std::max(0, std::min(w - 1, x));
    y = std::max(0, std::min(h - 1, y));
    return pixels[y * w + x];
}

Vec3 Texture::sample(float u, float v) const {
    if (w == 0 || h == 0) return {1, 1, 1};
    int x = (int)(u * w) % w; if (x < 0) x += w;
    int y = (int)(v * h) % h; if (y < 0) y += h;
    return pixels[y * w + x];
}

Vec3 Texture::sampleBilinear(float u, float v) const {
    if (w == 0 || h == 0) return {1, 1, 1};
    // Wrap
    u = u - std::floor(u);
    v = v - std::floor(v);
    float fx = u * w - 0.5f;
    float fy = v * h - 0.5f;
    int ix = (int)std::floor(fx);
    int iy = (int)std::floor(fy);
    float sx = fx - ix;
    float sy = fy - iy;

    Vec3 t00 = texel(ix,     iy);
    Vec3 t10 = texel(ix + 1, iy);
    Vec3 t01 = texel(ix,     iy + 1);
    Vec3 t11 = texel(ix + 1, iy + 1);

    return (t00 * (1 - sx) + t10 * sx) * (1 - sy) +
           (t01 * (1 - sx) + t11 * sx) * sy;
}

// ---------------------------------------------------------------------------
// Procedural textures
// ---------------------------------------------------------------------------
Texture Texture::checkerboard(int size, int squares) {
    Texture t;
    t.w = size; t.h = size;
    t.pixels.resize(size * size);
    float cell = (float)size / squares;
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            int cx = (int)(x / cell);
            int cy = (int)(y / cell);
            bool white = (cx + cy) % 2 == 0;
            t.pixels[y * size + x] = white ? Vec3{0.9f, 0.85f, 0.7f}
                                            : Vec3{0.15f, 0.12f, 0.1f};
        }
    }
    return t;
}

Texture Texture::gradient(int w, int h) {
    Texture t;
    t.w = w; t.h = h;
    t.pixels.resize(w * h);
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            float u = (float)x / w;
            float v = (float)y / h;
            t.pixels[y * w + x] = {u, v, 0.5f * (1 - u) * (1 - v)};
        }
    }
    return t;
}

// ---------------------------------------------------------------------------
// Load PPM P6 texture
// ---------------------------------------------------------------------------
bool Texture::loadPPM(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) { fprintf(stderr, "cannot open texture: %s\n", path.c_str()); return false; }

    std::string hdr; int maxval;
    file >> hdr >> w >> h >> maxval;
    file.get(); // consume newline after maxval

    if (hdr != "P6" || maxval != 255) {
        fprintf(stderr, "unsupported PPM format: %s max=%d\n", hdr.c_str(), maxval);
        return false;
    }

    pixels.resize(w * h);
    std::vector<unsigned char> row(w * 3);
    for (int y = 0; y < h; y++) {
        file.read((char*)row.data(), w * 3);
        for (int x = 0; x < w; x++) {
            pixels[y * w + x] = {row[x*3]/255.0f, row[x*3+1]/255.0f, row[x*3+2]/255.0f};
        }
    }
    printf("Loaded texture: %s  (%dx%d)\n", path.c_str(), w, h);
    return true;
}
