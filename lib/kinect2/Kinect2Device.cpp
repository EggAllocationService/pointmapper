//
// Created by Kyle Smith on 2026-05-25.
//

#include "pointmapper/kinect2/Kinect2Device.h"
#include <libfreenect2/packet_pipeline.h>
#include <libfreenect2/logger.h>

#include "pointmapper/kinect2/Kinect2Frame.h"

libfreenect2::Freenect2* Kinect2Device::freenect2 = nullptr;

Kinect2Device::Kinect2Device() {
    if (freenect2 == nullptr) {
        libfreenect2::setGlobalLogger(nullptr);
        freenect2 = new libfreenect2::Freenect2();
    }

    listener = new libfreenect2::SyncMultiFrameListener(libfreenect2::Frame::Type::Color | libfreenect2::Frame::Type::Ir | libfreenect2::Frame::Type::Depth);
    pipeline = new libfreenect2::OpenCLPacketPipeline();
    dev = freenect2->openDefaultDevice(pipeline);

    dev->setColorFrameListener(this->listener);
    dev->setIrAndDepthFrameListener(this->listener);
    dev->start();
}

Kinect2Device::Kinect2Device(const std::string &serial) {
    if (freenect2 == nullptr) {
        libfreenect2::setGlobalLogger(nullptr);
        freenect2 = new libfreenect2::Freenect2();
    }

    listener = new libfreenect2::SyncMultiFrameListener(libfreenect2::Frame::Type::Color | libfreenect2::Frame::Type::Ir | libfreenect2::Frame::Type::Depth);
    pipeline = new libfreenect2::OpenCLPacketPipeline();
    dev = freenect2->openDevice(serial, pipeline);

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
        .colorHeight = 1080,
        .colorType = BGRX
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

std::vector<std::string> Kinect2Device::EnumerateDevices() {
    if (freenect2 == nullptr) {
        libfreenect2::setGlobalLogger(nullptr);
        freenect2 = new libfreenect2::Freenect2();
    }

    auto count = freenect2->enumerateDevices();

    std::vector<std::string> strings(count);
    for (int i = 0; i < count; i++) {
        strings[i] = freenect2->getDeviceSerialNumber(i);
    }

    return std::move(strings);
}
