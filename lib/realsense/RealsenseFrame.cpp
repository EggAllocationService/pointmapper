//
// Created by Kyle Smith on 2026-05-21.
//

#include "RealsenseFrame.h"

const float * RealsenseFrame::GetDepth() {
    auto frame = frames.get_depth_frame();
    return static_cast<const float*>(frame.get_data());
}

const uint32_t * RealsenseFrame::GetColor() {
    auto frame = frames.get_color_frame();
    return static_cast<const uint32_t*>(frame.get_data());
}

float RealsenseFrame::GetDepthUnits() {
    return frames.get_depth_frame().get_units();
}

RealsenseFrame::~RealsenseFrame() = default;