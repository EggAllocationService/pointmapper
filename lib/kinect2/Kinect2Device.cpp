//
// Created by Kyle Smith on 2026-05-25.
//

#include "Kinect2Device.h"
#include <libfreenect2/packet_pipeline.h>

#include "Kinect2Frame.h"

Kinect2Device::Kinect2Device() {
    freenect2 = new libfreenect2::Freenect2();

    listener = new libfreenect2::SyncMultiFrameListener(libfreenect2::Frame::Type::Color | libfreenect2::Frame::Type::Ir | libfreenect2::Frame::Type::Depth);
    pipeline = new libfreenect2::OpenCLPacketPipeline();
    dev = freenect2->openDefaultDevice(pipeline);

    dev->setColorFrameListener(this->listener);
    dev->setIrAndDepthFrameListener(this->listener);

    dev->start();
}

CameraParams Kinect2Device::GetCameraParameters() {
    auto params = dev->getIrCameraParams();
    return {
        .fx = params.fx,
        .fy = params.fy,
        .cx = params.cx,
        .cy = params.cy,
        .width = 512,
        .height = 424,
        .colorWidth = 1920,
        .colorHeight = 1080
    };
}

std::string Kinect2Device::GetName() {
    return dev->getSerialNumber();
}

std::shared_ptr<Frame> Kinect2Device::GetNextFrame() {
    libfreenect2::FrameMap map;
    listener->waitForNewFrame(map);

    return std::make_shared<Kinect2Frame>(std::move(map), listener);
}
