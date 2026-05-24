#include "model.hpp"
#include <fstream>
#include <sstream>
#include <cstdio>

// ---------------------------------------------------------------------------
// OBJ loader
// ---------------------------------------------------------------------------
Mesh loadOBJ(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        fprintf(stderr, "ERROR: cannot open %s\n", path.c_str());
        return {};
    }

    std::vector<Vec3> tempPos;
    std::vector<Vec3> tempNrm;
    std::vector<Vec2> tempTex;
    Mesh mesh;

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        std::istringstream iss(line);
        std::string type;
        iss >> type;

        if (type == "v") {
            Vec3 v;
            iss >> v.x >> v.y >> v.z;
            tempPos.push_back(v);
        } else if (type == "vt") {
            Vec2 t;
            iss >> t.x >> t.y;
            tempTex.push_back({t.x, 1.0f - t.y}); // flip V for image textures
        } else if (type == "vn") {
            Vec3 n;
            iss >> n.x >> n.y >> n.z;
            tempNrm.push_back(n);
        } else if (type == "f") {
            std::vector<std::string> tokens;
            std::string tok;
            while (iss >> tok) tokens.push_back(tok);
            if (tokens.size() < 3) continue;

            // Fan triangulation
            for (size_t i = 1; i + 1 < tokens.size(); i++) {
                const size_t faceIdx[3] = {0, i, i + 1};
                for (size_t idx : faceIdx) {
                    const std::string& t = tokens[idx];
                    int vi = -1, ti = -1, ni = -1;

                    size_t slash1 = t.find('/');
                    if (slash1 == std::string::npos) {
                        vi = std::stoi(t) - 1;
                    } else {
                        vi = std::stoi(t.substr(0, slash1)) - 1;
                        size_t slash2 = t.find('/', slash1 + 1);
                        if (slash2 == std::string::npos) {
                            // v/vt
                            ti = std::stoi(t.substr(slash1 + 1)) - 1;
                        } else {
                            // v/vt/vn   or   v//vn
                            std::string mid = t.substr(slash1 + 1, slash2 - slash1 - 1);
                            if (!mid.empty()) ti = std::stoi(mid) - 1;
                            ni = std::stoi(t.substr(slash2 + 1)) - 1;
                        }
                    }

                    if (vi < 0 || vi >= (int)tempPos.size()) continue;

                    mesh.positions.push_back(tempPos[vi]);
                    mesh.normals.push_back(ni >= 0 && ni < (int)tempNrm.size()
                                               ? tempNrm[ni]
                                               : Vec3{0, 1, 0});
                    mesh.texcoords.push_back(ti >= 0 && ti < (int)tempTex.size()
                                                 ? tempTex[ti]
                                                 : Vec2{0, 0});
                    mesh.indices.push_back((int)mesh.positions.size() - 1);
                }
            }
        }
    }

    // If no normals, compute per-face flat normals
    if (tempNrm.empty()) {
        for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
            int i0 = mesh.indices[i], i1 = mesh.indices[i + 1], i2 = mesh.indices[i + 2];
            Vec3 a = mesh.positions[i1] - mesh.positions[i0];
            Vec3 b = mesh.positions[i2] - mesh.positions[i0];
            Vec3 n = a.cross(b).normalized();
            mesh.normals[i0] = mesh.normals[i1] = mesh.normals[i2] = n;
        }
    }

    printf("Loaded OBJ: %zu triangles\n", mesh.indices.size() / 3);
    return mesh;
}

// ---------------------------------------------------------------------------
// Procedural geometry
// ---------------------------------------------------------------------------
static void addQuad(Mesh& m,
                    const Vec3& v0, const Vec3& v1, const Vec3& v2, const Vec3& v3,
                    const Vec3& n,
                    const Vec2& t0, const Vec2& t1, const Vec2& t2, const Vec2& t3)
{
    int base = (int)m.positions.size();
    m.positions.insert(m.positions.end(), {v0, v1, v2, v3});
    m.normals.insert(m.normals.end(), {n, n, n, n});
    m.texcoords.insert(m.texcoords.end(), {t0, t1, t2, t3});
    m.indices.insert(m.indices.end(), {base, base + 1, base + 2,
                                       base, base + 2, base + 3});
}

Mesh createCube(float size) {
    float h = size * 0.5f;
    Mesh m;
    Vec2 uv[4] = {{0,0}, {1,0}, {1,1}, {0,1}};
    addQuad(m, {-h, -h,  h}, { h, -h,  h}, { h,  h,  h}, {-h,  h,  h}, { 0, 0, 1}, uv[0],uv[1],uv[2],uv[3]);
    addQuad(m, { h, -h, -h}, {-h, -h, -h}, {-h,  h, -h}, { h,  h, -h}, { 0, 0,-1}, uv[0],uv[1],uv[2],uv[3]);
    addQuad(m, {-h,  h,  h}, { h,  h,  h}, { h,  h, -h}, {-h,  h, -h}, { 0, 1, 0}, uv[0],uv[1],uv[2],uv[3]);
    addQuad(m, {-h, -h, -h}, { h, -h, -h}, { h, -h,  h}, {-h, -h,  h}, { 0,-1, 0}, uv[0],uv[1],uv[2],uv[3]);
    addQuad(m, { h, -h,  h}, { h, -h, -h}, { h,  h, -h}, { h,  h,  h}, { 1, 0, 0}, uv[0],uv[1],uv[2],uv[3]);
    addQuad(m, {-h, -h, -h}, {-h, -h,  h}, {-h,  h,  h}, {-h,  h, -h}, {-1, 0, 0}, uv[0],uv[1],uv[2],uv[3]);
    return m;
}

Mesh createPlane(float size, float uvScale) {
    float h = size * 0.5f;
    float s = uvScale;
    Mesh m;
    addQuad(m, {-h, 0, -h}, {h, 0, -h}, {h, 0, h}, {-h, 0, h}, {0, 1, 0},
            {0,0}, {s,0}, {s,s}, {0,s});
    return m;
}

Mesh createSphere(float radius, int segments) {
    Mesh m;
    int rings = segments / 2;

    for (int i = 0; i <= rings; i++) {
        float phi = 3.14159265f * (float)i / rings;
        float y = std::cos(phi) * radius;
        float r = std::sin(phi) * radius;

        for (int j = 0; j <= segments; j++) {
            float theta = 2.0f * 3.14159265f * (float)j / segments;
            float x = r * std::cos(theta);
            float z = r * std::sin(theta);
            m.positions.push_back({x, y, z});
            m.normals.push_back({x / radius, y / radius, z / radius});
            m.texcoords.push_back({(float)j / segments, (float)i / rings});
        }
    }

    for (int i = 0; i < rings; i++) {
        for (int j = 0; j < segments; j++) {
            int a = i * (segments + 1) + j;
            int b = a + segments + 1;
            int c = a + 1;
            int d = b + 1;
            m.indices.insert(m.indices.end(), {a, b, c, c, b, d});
        }
    }
    return m;
}
