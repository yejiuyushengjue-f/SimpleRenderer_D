#pragma once
#include "math.hpp"
#include <vector>
#include <string>

struct Mesh {
    std::vector<Vec3> positions;
    std::vector<Vec3> normals;
    std::vector<Vec2> texcoords;
    std::vector<int>  indices;
};

Mesh loadOBJ(const std::string& path);
Mesh createCube(float size = 1.0f);
Mesh createPlane(float size = 5.0f, float uvScale = 1.0f);
Mesh createSphere(float radius = 1.0f, int segments = 16);
