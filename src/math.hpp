#pragma once
#include <cmath>
#include <cassert>

struct Vec2 {
    float x = 0, y = 0;

    Vec2() = default;
    Vec2(float x, float y) : x(x), y(y) {}

    Vec2 operator+(const Vec2& o) const { return {x + o.x, y + o.y}; }
    Vec2 operator-(const Vec2& o) const { return {x - o.x, y - o.y}; }
    Vec2 operator*(float s) const { return {x * s, y * s}; }
    Vec2 operator/(float s) const { float inv = 1.0f / s; return {x * inv, y * inv}; }
    Vec2 operator*(const Vec2& o) const { return {x * o.x, y * o.y}; }

    float& operator[](int i) { return (&x)[i]; }
    const float& operator[](int i) const { return (&x)[i]; }
};

inline Vec2 operator*(float s, const Vec2& v) { return v * s; }

struct Vec3 {
    float x = 0, y = 0, z = 0;

    Vec3() = default;
    Vec3(float x, float y, float z) : x(x), y(y), z(z) {}

    Vec3 operator+(const Vec3& o) const { return {x + o.x, y + o.y, z + o.z}; }
    Vec3 operator-(const Vec3& o) const { return {x - o.x, y - o.y, z - o.z}; }
    Vec3 operator*(float s) const { return {x * s, y * s, z * s}; }
    Vec3 operator/(float s) const { float inv = 1.0f / s; return {x * inv, y * inv, z * inv}; }
    Vec3 operator*(const Vec3& o) const { return {x * o.x, y * o.y, z * o.z}; }
    Vec3 operator-() const { return {-x, -y, -z}; }

    Vec3& operator+=(const Vec3& o) { x += o.x; y += o.y; z += o.z; return *this; }
    Vec3& operator*=(float s) { x *= s; y *= s; z *= s; return *this; }

    float dot(const Vec3& o) const { return x * o.x + y * o.y + z * o.z; }
    Vec3 cross(const Vec3& o) const {
        return {y * o.z - z * o.y, z * o.x - x * o.z, x * o.y - y * o.x};
    }
    float length() const { return std::sqrt(x * x + y * y + z * z); }
    float lengthSq() const { return x * x + y * y + z * z; }
    Vec3 normalized() const {
        float len = length();
        return len > 1e-8f ? *this / len : Vec3{0, 0, 0};
    }

    float& operator[](int i) { return (&x)[i]; }
    const float& operator[](int i) const { return (&x)[i]; }
};

inline Vec3 operator*(float s, const Vec3& v) { return v * s; }

struct Vec4 {
    float x = 0, y = 0, z = 0, w = 0;

    Vec4() = default;
    Vec4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
    Vec4(const Vec3& v, float w) : x(v.x), y(v.y), z(v.z), w(w) {}

    Vec3 xyz() const { float inv = 1.0f / w; return {x * inv, y * inv, z * inv}; }

    float& operator[](int i) { return (&x)[i]; }
    const float& operator[](int i) const { return (&x)[i]; }
};

struct Mat4 {
    float m[4][4] = {};

    static Mat4 identity() {
        Mat4 r;
        r.m[0][0] = r.m[1][1] = r.m[2][2] = r.m[3][3] = 1.0f;
        return r;
    }

    static Mat4 translate(float x, float y, float z) {
        Mat4 r = identity();
        r.m[0][3] = x;
        r.m[1][3] = y;
        r.m[2][3] = z;
        return r;
    }

    static Mat4 scale(float x, float y, float z) {
        Mat4 r;
        r.m[0][0] = x;
        r.m[1][1] = y;
        r.m[2][2] = z;
        r.m[3][3] = 1.0f;
        return r;
    }

    static Mat4 rotateX(float angle) {
        float c = std::cos(angle), s = std::sin(angle);
        Mat4 r = identity();
        r.m[1][1] = c; r.m[1][2] = -s;
        r.m[2][1] = s; r.m[2][2] = c;
        return r;
    }

    static Mat4 rotateY(float angle) {
        float c = std::cos(angle), s = std::sin(angle);
        Mat4 r = identity();
        r.m[0][0] = c;  r.m[0][2] = s;
        r.m[2][0] = -s; r.m[2][2] = c;
        return r;
    }

    static Mat4 rotateZ(float angle) {
        float c = std::cos(angle), s = std::sin(angle);
        Mat4 r = identity();
        r.m[0][0] = c; r.m[0][1] = -s;
        r.m[1][0] = s; r.m[1][1] = c;
        return r;
    }

    // fovY in radians
    static Mat4 perspective(float fovY, float aspect, float near, float far) {
        float f = 1.0f / std::tan(fovY * 0.5f);
        Mat4 r;
        r.m[0][0] = f / aspect;
        r.m[1][1] = f;
        r.m[2][2] = (far + near) / (near - far);
        r.m[2][3] = (2.0f * far * near) / (near - far);
        r.m[3][2] = -1.0f;
        return r;
    }

    static Mat4 lookAt(const Vec3& eye, const Vec3& center, const Vec3& up) {
        Vec3 f = (center - eye).normalized();
        Vec3 s = f.cross(up.normalized()).normalized();
        Vec3 u = s.cross(f);

        Mat4 r = identity();
        r.m[0][0] = s.x;  r.m[0][1] = s.y;  r.m[0][2] = s.z;
        r.m[1][0] = u.x;  r.m[1][1] = u.y;  r.m[1][2] = u.z;
        r.m[2][0] = -f.x; r.m[2][1] = -f.y; r.m[2][2] = -f.z;
        r.m[0][3] = -s.dot(eye);
        r.m[1][3] = -u.dot(eye);
        r.m[2][3] = f.dot(eye);
        return r;
    }

    Mat4 operator*(const Mat4& o) const {
        Mat4 r;
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++)
                r.m[i][j] = m[i][0] * o.m[0][j] + m[i][1] * o.m[1][j] +
                            m[i][2] * o.m[2][j] + m[i][3] * o.m[3][j];
        return r;
    }

    Vec4 operator*(const Vec4& v) const {
        return {
            m[0][0] * v.x + m[0][1] * v.y + m[0][2] * v.z + m[0][3] * v.w,
            m[1][0] * v.x + m[1][1] * v.y + m[1][2] * v.z + m[1][3] * v.w,
            m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z + m[2][3] * v.w,
            m[3][0] * v.x + m[3][1] * v.y + m[3][2] * v.z + m[3][3] * v.w,
        };
    }

    // Transform a direction vector (no translation)
    Vec3 transformDir(const Vec3& v) const {
        return {
            m[0][0] * v.x + m[0][1] * v.y + m[0][2] * v.z,
            m[1][0] * v.x + m[1][1] * v.y + m[1][2] * v.z,
            m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z,
        };
    }
};
