//
// Created by Kyle Smith on 2026-05-21.
//
#pragma once
#include <cstdint>


struct CameraParams {
    float fx, fy, cx, cy;
    int width, height;
};

enum DepthType {
    F16,
    F32
};

/// A single frame of color + depth data
class Frame {
public:
    virtual ~Frame();

    virtual const float* GetDepth() = 0;
    virtual const uint32_t* GetColor() = 0;

    virtual float GetDepthUnits() = 0;
    virtual DepthType GetDepthType() = 0;
};