//
// Created by Kyle Smith on 2026-05-25.
//
#pragma once
#include "../DepthDevice.h"
#include <libfreenect2/libfreenect2.hpp>
#include <libfreenect2/frame_listener_impl.h>
#include <libfreenect2/registration.h>


class Kinect2Device: public DepthDevice {
public:
    /// Open the first kinect device found.
    Kinect2Device();

    /// Open a specific device with the given serial number.
    /// May throw an exception if the device in question does not exist.
    Kinect2Device(const std::string& serial);

    CameraParams GetCameraParameters() override;

    /// Get the Kinect's name, likely to be the serial number.
    std::string GetName() override;

    std::shared_ptr<Frame> GetNextFrame() override;

    /// Retrieves the serial numbers of each connected Kinect 2
    /// @return A list of device serial numbers
    static std::vector<std::string> EnumerateDevices();

    [[nodiscard]] libfreenect2::Freenect2Device *GetHandle() const {
        return dev;
    }

private:
    static libfreenect2::Freenect2* freenect2;
    libfreenect2::Freenect2Device *dev;
    libfreenect2::PacketPipeline *pipeline;
    libfreenect2::SyncMultiFrameListener *listener;

};
