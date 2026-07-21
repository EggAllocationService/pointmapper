//
// Created by Kyle Smith on 2026-05-21.
//
#pragma once
#include <cstdint>
#ifdef POINTMAPPER_USE_GLENGINE_MATH
#include "Vectors.h"
#else
#include "PointmapperMath.h"
#endif

// Opaque pointer for a GPU-side cloud data structure
typedef void* PointmapperDeviceCloud;


struct PointXYZRGB {
    float x, y, z;
    union {
        float w;
        unsigned char r, g, b, a;
        uint32_t color;
    };
};
enum ColorType {
    RGBX,
    BGRX,
};


struct CameraParams {
    float fx, fy, cx, cy;
    int width, height;
    int colorWidth, colorHeight;
    ColorType colorType;
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
    virtual float4 GetAxisScale() = 0;
};