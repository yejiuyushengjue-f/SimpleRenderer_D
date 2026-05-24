#pragma once
#include "math.hpp"
#include <vector>
#include <string>

class Texture {
public:
    Texture() = default;

    int width()  const { return w; }
    int height() const { return h; }

    // Nearest-neighbour sample  (u,v in [0,1])
    Vec3 sample(float u, float v) const;
    // Bilinear sample
    Vec3 sampleBilinear(float u, float v) const;

    // Procedural factories
    static Texture checkerboard(int size, int squares);
    static Texture gradient(int w, int h);

    // File I/O
    bool loadPPM(const std::string& path);
    const std::vector<Vec3>& data() const { return pixels; }

private:
    int w = 0, h = 0;
    std::vector<Vec3> pixels;
    Vec3 texel(int x, int y) const;
};
