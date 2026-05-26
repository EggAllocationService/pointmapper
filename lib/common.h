//
// Created by Kyle Smith on 2026-05-21.
//
#pragma once
#include <cstdint>

// Opaque pointer for a GPU-side cloud data structure
typedef void* PointmapperDeviceCloud;


struct alignas(32) PointXYZRGB {
    float x, y, z, w;
    union {
        unsigned char r, g, b, a;
        uint32_t color;
    };
};
struct CameraParams {
    float fx, fy, cx, cy;
    int width, height;
    int colorWidth, colorHeight;
};

enum DepthType {
    Z16,
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