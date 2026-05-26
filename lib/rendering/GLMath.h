//
// Created by Kyle Smith on 2025-09-29.
//
#pragma once

#include "Matrix.h"
#include <vector>
#include <span>
#include <memory>

#define PI 3.14159265358979323846

namespace glengine::math {
    /// Creates a 2d clockwise rotation matrix:
    /// cos theta, -sin theta, 0
    /// sin theta, cos theta , 0
    /// 0        , 0         , 1
    /// `theta` is an angle in radians
    mat3 rotation2D(float theta);

    /// Creates a scale matrix:
    /// scale.x, 0      , 0
    /// 0      , scale.y, 0
    /// 0      , 0      , 1
    mat3 scale2D(float2 scale);

    /// Creates a translation matrix:
    /// 1, 0, translation.x
    /// 0, 1, translation.y
    /// 0, 0, 1
    mat3 translate2D(float2 translation);

    /// Creates a unit quaternion rotation from
    /// a set of euler angles.
    float4 quatFromEuler(float3 angles);

    /// Linear interpolation, templated so it can be used with vectors
    template<typename T>
    T lerp(T a, T b, float c) {
        return (a * (1 - c)) + (b * c);
    }

    /// Creates a world -> eye matrix from the absolute
    /// transform matrix of a camera component
    mat4 viewMatrix(mat4 cameraComponentMatrix);

    /// Creates a perspective projection matrix for a left-handed
    /// coordinate system (+z is into screen, +x is right, +y is up)
    mat4 perspectiveMatrix(float fov, float aspect, float near, float far);

    /// Random floating-point number in range
    float frand(float min, float max);

    /// Random floating-point number in [0, 1]
    float frand();
}
