//
// Created by Kyle Smith on 2026-05-25.
//

#include "Kinect2Frame.h"

Kinect2Frame::Kinect2Frame(libfreenect2::FrameMap &&map, libfreenect2::SyncMultiFrameListener* listener) {
    this->map = std::move(map);
    this->listener = listener;
}

Kinect2Frame::~Kinect2Frame() {
    listener->release(map);
}

const float * Kinect2Frame::GetDepth() {
    return reinterpret_cast<const float*>(this->map[libfreenect2::Frame::Type::Depth]->data);
}

const uint32_t * Kinect2Frame::GetColor() {
    return reinterpret_cast<u_int32_t*>(this->map[libfreenect2::Frame::Type::Color]->data);
}

float Kinect2Frame::GetDepthUnits() {
    return 1000; // Kinect depth is always in millimeters
}

DepthType Kinect2Frame::GetDepthType() {
    return DepthType::F32;
}
