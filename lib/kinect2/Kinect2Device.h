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
    Kinect2Device();

    CameraParams GetCameraParameters() override;

    std::string GetName() override;

    std::shared_ptr<Frame> GetNextFrame() override;

private:
    libfreenect2::Freenect2* freenect2;
    libfreenect2::Freenect2Device *dev;
    libfreenect2::PacketPipeline *pipeline;
    libfreenect2::SyncMultiFrameListener *listener;

};
