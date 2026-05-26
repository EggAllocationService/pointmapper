//
// Created by Kyle Smith on 2025-10-17.
//
#pragma once
#include "Matrix.h"
#include "Vectors.h"

class Transform {
    public:
    /// By default, initializes an identity transform
    Transform();

    /// Sets local rotation in x, y, z space
    /// Axis-Angle format, with components in degrees
    /// Rotation is done in X Y Z order
    void SetRotation(float3);

    /// Gets the local rotation as euler angles
    [[nodiscard]] float3 GetRotation() const {
        return rotation;
    }

    /// Sets the transform's scale
    /// X Y Z, straight multiplier
    void SetScale(float3);

    /// Gets current scale
    [[nodiscard]] float3 GetScale() const {
        return scale;
    }

    /// Sets position relative to parent
    void SetPosition(float3);

    /// Gets position relative to parent
    [[nodiscard]] float3 GetPosition() const {
        return position;
    }

    /// Sets the parent transform
    void SetParent(Transform *parent);

    /// Gets a 4x4 matrix representing this transform
    /// Does not include the parent's transform
    [[nodiscard]] mat4 GetMatrix() const;

    /// Gets a pointer to the local column-major 4x4 matrix
    const float* GetMatrixPointer();

    /// Converts this transform to a 4x4 matrix
    /// Will be combined with all parent matrices
    [[nodiscard]] mat4 GetAbsoluteMatrix() const;

    /// Computes the forward (+Z) vector of this transform in local space
    [[nodiscard]] float3 GetForwardVector();

    /// Computes the Up (+Y) vector of this transform in local space
    [[nodiscard]] float3 GetUpVector();

    /// Computes the right (+X) vector of this transform in local space
    [[nodiscard]] float3 GetRightVector();
private:
    mat4 cachedMatrix;
    float3 rotation;
    float3 scale;
    float3 position;
    Transform *parent;

    void RecalculateMatrix();
};
