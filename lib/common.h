//
// Created by Kyle Smith on 2026-05-21.
//
#pragma once


struct CameraParams {
    float fx, fy, cx, cy;
    int width, height;
};

/// A single frame of color + depth data
class Frame {
    virtual ~Frame() = 0;

    virtual float* GetDepth() = 0;
    virtual uint32_t* GetColor() = 0;

    virtual float GetDepthUnits() = 0;
};