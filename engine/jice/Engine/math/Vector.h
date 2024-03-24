#pragma once

#include <cmath>
#include <iostream>

class Vec3 {
public:
    union {
        struct {
            float x, y, z;
        };
        float data[3];
        struct {
            float r, g, b;
        };
        struct {
            float s, t, p;
        };
        struct {
            float u, v, w;
        };
    };
    inline Vec3() : x(0), y(0), z(0) {}
    inline Vec3(float x, float y, float z) : x(x), y(y), z(z) {}
    inline Vec3 operator+(const Vec3 &other) const {
        return {x + other.x, y + other.y, z + other.z};
    }
    inline Vec3 operator-(const Vec3 &other) const {
        return {x - other.x, y - other.y, z - other.z};
    }
    inline Vec3 operator*(float scalar) const {
        return {x * scalar, y * scalar, z * scalar};
    }
    inline Vec3 operator/(float scalar) const {
        return {x / scalar, y / scalar, z / scalar};
    }
    inline Vec3 &operator+=(const Vec3 &other) {
        x += other.x;
        y += other.y;
        z += other.z;
        return *this;
    }
    inline Vec3 &operator-=(const Vec3 &other) {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        return *this;
    }
    inline Vec3 &operator*=(float scalar) {
        x *= scalar;
        y *= scalar;
        z *= scalar;
        return *this;
    }
    inline Vec3 &operator/=(float scalar) {
        x /= scalar;
        y /= scalar;
        z /= scalar;
        return *this;
    }
    inline bool operator==(const Vec3 &other) const {
        return x == other.x && y == other.y && z == other.z;
    }
    inline bool operator!=(const Vec3 &other) const {
        return x != other.x || y != other.y || z != other.z;
    }
    [[nodiscard]] inline float dot(const Vec3 &other) const {
        return x * other.x + y * other.y + z * other.z;
    }
    [[nodiscard]] inline Vec3 cross(const Vec3 &other) const {
        return {y * other.z - z * other.y, z * other.x - x * other.z, x * other.y - y * other.x};
    }
    [[nodiscard]] inline float magnitude() const {
        return sqrtf(x * x + y * y + z * z);
    }
    [[nodiscard]] inline Vec3 normalized() const {
        return *this / magnitude();
    }
    inline void normalize() {
        *this /= magnitude();
    }
    [[nodiscard]] inline float distance(const Vec3 &other) const {
        return (*this - other).magnitude();
    }
    [[nodiscard]] inline float distanceSquared(const Vec3 &other) const {
        return (*this - other).dot(*this - other);
    }
    [[nodiscard]] inline Vec3 lerp(const Vec3 &other, float t) const {
        return *this + (other - *this) * t;
    }
    [[nodiscard]] inline Vec3 slerp(const Vec3 &other, float t) const {
        float dot = normalized().dot(other.normalized());
        dot = fmaxf(fminf(dot, 1), -1);
        float theta = acosf(dot) * t;
        Vec3 relative = other - *this * dot;
        relative.normalize();
        return *this * cosf(theta) + relative * sinf(theta);
    }
    [[nodiscard]] inline Vec3 nlerp(const Vec3 &other, float t) const {
        return lerp(other, t).normalized();
    }
    [[nodiscard]] inline Vec3 reflect(const Vec3 &normal) const {
        return *this - normal * 2 * dot(normal);
    }

    friend std::ostream &operator<<(std::ostream &os, const Vec3 &vec3) {
        os << "Vec3(" << vec3.x << ", " << vec3.y << ", " << vec3.z << ")";
        return os;
    }
};

