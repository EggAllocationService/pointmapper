//
// Created by Kyle Smith on 2025-10-17.
//

#include "Transform.h"

Transform::Transform() {
    scale = float3(1, 1, 1);
    parent = nullptr;
    RecalculateMatrix();
}

void Transform::SetRotation(float3 rot) {
    this->rotation = rot;
    RecalculateMatrix();
}

void Transform::SetScale(float3 scale) {
    this->scale = scale;
    RecalculateMatrix();
}

void Transform::SetPosition(float3 pos) {
    this->position = pos;
    RecalculateMatrix();
}

void Transform::SetParent(Transform *parent) {
    this->parent = parent;
}

mat4 Transform::GetMatrix() const {
    return cachedMatrix;
}

const float *Transform::GetMatrixPointer() {
    return reinterpret_cast<float *>(&cachedMatrix);
}

mat4 Transform::GetAbsoluteMatrix() const {
    if (parent != nullptr) {
        return parent->GetAbsoluteMatrix() * cachedMatrix;
    }

    return cachedMatrix;
}

float3 Transform::GetForwardVector() {
    auto zVec = *cachedMatrix[2];

    return float3(zVec[0], zVec[1], zVec[2]).norm();
}

float3 Transform::GetUpVector() {
    auto yVec = *cachedMatrix[1];
    return float3(yVec[0], yVec[1], yVec[2]).norm();
}

float3 Transform::GetRightVector() {
    auto xVec = *cachedMatrix[0];
    return float3(xVec[0], xVec[1], xVec[2]).norm();
}

void Transform::RecalculateMatrix() {
    // a bit of a messy method, but it's fast
    // directly computes a combined rotation, scale, and translation matrix
    cachedMatrix = mat4::identity();

    float cx = cosf(rotation.x);
    float sx = sinf(rotation.x);
    float cy = cosf(rotation.y);
    float sy = sinf(rotation.y);
    float cz = cosf(rotation.z);
    float sz = sinf(rotation.z);

    auto row0 = cachedMatrix[0];
    row0->set(0, (cy*cz + sy*sz*sx) * scale.x);
    row0->set(1, (cx*sz) * scale.x);
    row0->set(2, (cy*sx*sz - cz*sy) * scale.x);

    auto row1 = cachedMatrix[1];
    row1->set(0, (cz*sy*sx - cy*sz) * scale.y);
    row1->set(1, (cx*cz) * scale.y);
    row1->set(2, (cy*cz*sx + sy*sz) * scale.y);

    auto row2 = cachedMatrix[2];
    row2->set(0, (cx*sy) * scale.z);
    row2->set(1, (-sx) * scale.z);
    row2->set(2, (cx * cy) * scale.z);

    auto row3 = cachedMatrix[3];
    row3->set(0, position.x / scale.x);
    row3->set(1, position.y / scale.y);
    row3->set(2, position.z / scale.z);
}
