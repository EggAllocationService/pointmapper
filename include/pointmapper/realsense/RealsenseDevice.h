//
// Created by Kyle Smith on 2026-05-21.
//
#pragma once
#include "../DepthDevice.h"
#include <librealsense2/rs.hpp>

#include "RealsenseFrame.h"

class RealsenseDevice : public DepthDevice {
public:
    ~RealsenseDevice() override = default;

    RealsenseDevice();
    RealsenseDevice(const std::string& serial);

    CameraParams GetCameraParameters() override;
    std::string GetName() override;
    std::shared_ptr<Frame> GetNextFrame() override;

    static std::vector<std::string> EnumerateDevices();
private:
    rs2::pipeline pipeline;
    std::shared_ptr<RealsenseFrame> frame;
};
