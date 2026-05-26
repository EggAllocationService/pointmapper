//
// Created by Kyle Smith on 2025-09-26.
//
#pragma once
#include <cmath>

/// Helper type to allow for glsl-like vector swizzling
/// @tparam vector the type of the swizzled vector, possibly narrowed from the source
/// @tparam primitive is the primitive type being stored in the vector, e.g. `float`
/// @tparam len is the number of lanes in the _source_ vector, *not* the swizzled vector.
///     The length of the swizzle is implicitly determined from the number of indices provided
/// @tparam indices are the mappings from the source lanes to each lane of the swizzled vector.
///
/// Example usage for a 3-wide int vector:
/// @code
/// union {
///     int data[3]; // the actual storage
///
///     // Setup parameters:
///     // `vec2<int>` indicates this swizzled representation can be converted to a vec2<int>
///     // `int` indicates the primitive type stored is an int, allowing for broadcast assignments (foo.xy = 3)
///     // `3` sets the source vector width to 3 (so it fits in the union correctly)
///     // `1, 0` indicates there are two elements in the swizzled vector, from source lane 1 and 0 respectively.
///     // Therefore, `yx` will be a reversed view of the first two lanes of the vector.
///     vec_swizzle<vec2<int>, int, 3, 0, 1> yx;
///
///     // We can also use `vec_swizzle` to easily allow narrowing a vector.
///     // Since `indices` is in ascending order here, `xy` will simply be the first two lanes of the source vector.
///     vec_swizzle<vec2<int>, int, 3, 0, 1> xy;
/// }
/// @endcode
template<typename vector, typename primitive, int len, int... indices>
struct vec_swizzle {
    primitive data[len];

    /********
    * Vector assignment operations
    *********/
#define SWIZVASSIGNOP(op) vector operator op (vector rhs) { \
        int i = 0; \
        ((data[indices] op rhs[i++]), ...); \
        return vector(data[indices]...); \
    }
    SWIZVASSIGNOP(=)
    SWIZVASSIGNOP(+=)
    SWIZVASSIGNOP(-=)
    SWIZVASSIGNOP(/=)
    SWIZVASSIGNOP(*=)

    /********
    * Vector arithmatic operations
    *********/
#define SWIZVARITHMATICOP(op) vector operator op (vector rhs) { \
        vector result; \
        int i = 0; \
        ((result.data[indices] = data[indices] op rhs[i++]), ...); \
        return result; \
    }

    SWIZVARITHMATICOP(+)
    SWIZVARITHMATICOP(-)
    SWIZVARITHMATICOP(/)
    SWIZVARITHMATICOP(*)

    /********
    * Scalar assignment operations
    *********/
#define SWIZSCALAROP(op) vector operator op (primitive rhs) { \
        return vector((data[indices] op rhs)...); \
    }

    vec_swizzle& operator=(primitive rhs) {
        vector((data[indices] = rhs)...);
        return *this;
    }
    SWIZSCALAROP(+=)
    SWIZSCALAROP(-=)
    SWIZSCALAROP(/=)
    SWIZSCALAROP(*=)

    /********
    * Scalar arithmatic operations
    *********/
    SWIZSCALAROP(+)
    SWIZSCALAROP(-)
    SWIZSCALAROP(*)
    SWIZSCALAROP(/)

    /// <summary>
    /// Converts this swizzled view to a vector
    /// </summary>
    operator vector() const {
        return vector(data[indices]...);
    }
};

/// Constant-length vector, templated.
///
/// Useful for math in other templated code, e.g. matrix multiplication
/// @tparam T primitive type of vector
/// @tparam LEN vector width
template<typename T, int LEN>
struct vecn {
    vecn() {
    }

    static vecn zero() {
        vecn ret;
        for (int i = 0; i < LEN; i++) ret.data[i] = 0;
        return ret;
    }

    T data[LEN];

    void set(int index, T value) {
        data[index] = value;
    }

    T &operator[](int index) {
        return data[index];
    }

    /// lane-wise multiply
    vecn operator*(const vecn &rhs) const {
        vecn result;
        for (int i = 0; i < LEN; i++) {
            result.data[i] = data[i] * rhs.data[i];
        }
        return result;
    }

    /// broadcast multiply
    template<typename I>
    vecn operator*(const I rhs) const {
        vecn result;

        for (int i = 0; i < LEN; i++) {
            result.data[i] = data[i] * rhs;
        }
        return result;
    }

    /// lane-wise addition
    vecn operator+(const vecn rhs) const {
        vecn result;

        for (int i = 0; i < LEN; i++) {
            result.data[i] = data[i] + rhs.data[i];
        }
        return result;
    }

    /// lane-wise addition + assignment
    void operator+=(const vecn rhs) {
        for (int i = 0; i < LEN; i++) {
            data[i] += rhs.data[i];
        }
    }
};

/// Constant-width vector, 2 lanes
/// @tparam T primitive type
template<typename T>
struct vec2 {
    vec2() {
        data[0] = 0;
        data[1] = 0;
    }

    vec2(T x, T y) {
        data[0] = x;
        data[1] = y;
    }

    template<typename O>
    vec2(vec2<O> other) {
        data[0] = other[0];
        data[1] = other[1];
    }

    union {
        T data[2];

        struct {
            T x, y;
        };

        struct {
            T r, g;
        };

        vec_swizzle<vec2, T, 2, 0, 1> xy, rg;
        vec_swizzle<vec2, T, 2, 1, 0> yx, gr;
        vec_swizzle<vec2, T, 2, 0, 0> xx, rr;
        vec_swizzle<vec2, T, 2, 1, 1> yy, gg;
    };

    operator T *() {
        return &data[0];
    }

    operator const T *() const {
        return &data[0];
    }

    bool operator==(const vec2 &rhs) {
        return data[0] == rhs.data[0] && data[1] == rhs.data[1];
    }

    vec2 operator+=(const vec2 rhs) {
        return vec2(x += rhs.x, y += rhs.y);
    }

    vec2 operator*(const vec2 rhs) const {
        return vec2(x * rhs.x, y * rhs.y);
    }
    vec2 operator*(const T rhs) const {
        return vec2(x * rhs, y * rhs);
    }

    vec2 operator/(const vec2 rhs) const {
        return vec2(x / rhs.x, y / rhs.y);
    }
    vec2 operator/(const T rhs) const {
        return vec2(x / rhs, y / rhs);
    }

    vec2 operator+(const vec2 rhs) const {
        return vec2(x + rhs.x, y + rhs.y);
    }
    vec2 operator+(const T rhs) const {
        return vec2(x + rhs, y + rhs);
    }

    vec2 operator -(const vec2 rhs) const {
        return vec2(x - rhs.x, y - rhs.y);
    }
    vec2 operator-(const T rhs) const {
        return vec2(x - rhs, y - rhs);
    }

    /// lane-wise numerical negation
    vec2 operator-() const {
        return vec2(-x, -y);
    }

    bool operator<(vec2 other) {
        return x < other.x && y < other.y;
    }

    /// length squared
    T len2() const {
        return x * x + y * y;
    }

    /// absolute length
    T len() const {
        return sqrt(len2());
    }

    T dot(const vec2 other) const {
        return x * other.x + y * other.y;
    }

    vec2 norm() const {
        return *this / len();
    }
};

/// Constant-width vector, 3 lanes
/// @tparam T primitive type
template<typename T>
struct vec3 {
    vec3() {
        data[0] = 0;
        data[1] = 0;
        data[2] = 0;
    }

    vec3(T x, T y, T z) {
        data[0] = x;
        data[1] = y;
        data[2] = z;
    }

    vec3(vec2<T> xy, T z) {
        data[0] = xy.x;
        data[1] = xy.y;
        data[2] = z;
    }

    template<typename O>
    vec3(vec3<O> other) {
        data[0] = T(other.x);
        data[1] = T(other.y);
        data[2] = T(other.z);
    }

    union {
        T data[3];

        struct {
            T x, y, z;
        };

        struct {
            T r, g, b;
        };

        vec_swizzle<vec3, T, 3, 0, 1, 2> xyz, rgb;
        vec_swizzle<vec3, T, 3, 2, 1, 0> zyx, bgr;
        vec_swizzle<vec2<T>, T, 3, 0, 1> xy, rg;
        vec_swizzle<vec2<T>, T, 3, 1, 0> yx, gr;
        vec_swizzle<vec2<T>, T, 3, 0, 2> xz, rb;
        vec_swizzle<vec2<T>, T, 3, 2, 0> zx, br;
    };

    operator T *() {
        return &data[0];
    }

    operator const T*() const {
        return &data[0];
    }

    bool operator==(const vec3 &rhs) const {
        for (int i = 0; i < 3; i++) {
            if (data[i] != rhs.data[i]) return false;
        }
        return true;
    }

    // TODO: surely these can be macros
    vec3 operator*(const vec3 rhs) const {
        return vec3(x * rhs.x, y * rhs.y, z * rhs.z);
    }

    vec3 operator*(const T rhs) const {
        return vec3(x * rhs, y * rhs, z * rhs);
    }

    vec3 operator/(const vec3 rhs) const {
        return vec3(x / rhs.x, y / rhs.y, z / rhs.z);
    }

    vec3 operator/(const T rhs) const {
        return vec3(x / rhs, y / rhs, z / rhs);
    }

    vec3 operator+(const vec3 rhs) const {
        return vec3(x + rhs.x, y + rhs.y, z + rhs.z);
    }

    vec3 operator+(const T rhs) const {
        return vec3(x + rhs, y + rhs, z + rhs);
    }

    vec3 operator-(const vec3 rhs) const {
        return vec3(x - rhs.x, y - rhs.y, z - rhs.z);
    }

    vec3 operator-(const T rhs) const {
        return vec3(x - rhs, y - rhs, z - rhs);
    }

    // cross product
    vec3 cross(const vec3 rhs) const {
        return vec3(
            y * rhs.z - z * rhs.y,
            z * rhs.x - x * rhs.z,
            x * rhs.y - y * rhs.x
        );
    }

    // length squared
    T len2() {
        return x*x + y*y + z*z;
    }

    T len() {
        return sqrt(len2());
    }

    vec3 norm() {
        return *this / len();
    }
};

/// Constant-width vector, 4 lanes
/// @tparam T primitive type
template<typename T>
struct vec4 {
    vec4() {
        data[0] = 0;
        data[1] = 0;
        data[2] = 0;
        data[3] = 0;
    }

    vec4(T x, T y, T z, T w) {
        data[0] = x;
        data[1] = y;
        data[2] = z;
        data[3] = w;
    }

    vec4(vec2<T> xy, T z, T w) {
        data[0] = xy.x;
        data[1] = xy.y;
        data[2] = z;
        data[3] = w;
    }

    vec4(vec3<T> xyz, T w) {
        data[0] = xyz.x;
        data[1] = xyz.y;
        data[2] = xyz.z;
        data[3] = w;
    }

    union {
        T data[4];

        struct {
            T x, y, z, w;
        };

        struct {
            T r, g, b, a;
        };

        vec_swizzle<vec4, T, 4, 0, 1, 2, 3> xyzw, rgba;
        vec_swizzle<vec4, T, 4, 3, 2, 1, 0> wzyx, abgr;
        vec_swizzle<vec3<T>, T, 4, 0, 1, 2> xyz, rgb;
        vec_swizzle<vec3<T>, T, 4, 2, 1, 0> zyx, bgr;
        vec_swizzle<vec2<T>, T, 4, 0, 1> xy, rg;
        vec_swizzle<vec2<T>, T, 4, 1, 0> yx, gr;
    };

    operator const T *() const {
        return &x;
    }

    vec4 operator *(const vec4 other) const {
        return vec4(x * other.x, y * other.y, z * other.z, w * other.w);
    }
    vec4 operator *(const T other) const {
        return vec4(x * other, y * other, z * other, w * other);
    }

    vec4 operator +(const vec4 other) const {
        return vec4(x + other.x, y + other.y, z + other.z, w + other.w);
    }

    void operator +=(const vec4 other) {
        for (int i = 0; i < 4; i++) {
            this->data[i] += other.data[i];
        }
    }

    bool operator==(const vec4 &other) const {
        for (int i = 0; i < 4; i++) {
            if (data[i] != other.data[i]) return false;
        }
        return true;
    }
};

// define some basic vector types for convenience
typedef vec4<float> float4;
typedef vec4<int> int4;

typedef vec3<float> float3;
typedef vec3<int> int3;

typedef vec2<float> float2;
typedef vec2<int> int2;

// utility methods
inline int2 floor(float2 a) {
    return int2((int)floor(a.x), (int)floor(a.y));
}