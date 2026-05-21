//
// Created by Kyle Smith on 2026-05-21.
//
#pragma once
#include <string>

#include "common.h"

class DepthDevice {
public:
    virtual ~DepthDevice() = default;

    virtual CameraParams GetCameraParameters() = 0;
    virtual std::string GetName() = 0;
    virtual std::shared_ptr<Frame> GetNextFrame() = 0;
};
