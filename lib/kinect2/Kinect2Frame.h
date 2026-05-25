//
// Created by Kyle Smith on 2026-05-25.
//
#pragma once
#include <libfreenect2/frame_listener_impl.h>

#include "../common.h"


class Kinect2Frame: public Frame {
public:
    Kinect2Frame(libfreenect2::FrameMap&& map, libfreenect2::SyncMultiFrameListener* listener);

    ~Kinect2Frame() override;

    const float * GetDepth() override;

    const uint32_t * GetColor() override;

    float GetDepthUnits() override;

    DepthType GetDepthType() override;

private:
    libfreenect2::FrameMap map;
    libfreenect2::SyncMultiFrameListener* listener;
};
