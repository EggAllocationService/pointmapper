//
// Created by Kyle Smith on 2026-05-21.
//

#include "RealsenseFrame.h"

const float * RealsenseFrame::GetDepth() {
    return converted_depth.data();
}

const uint32_t * RealsenseFrame::GetColor() {
    auto frame = frames.get_color_frame();
    return static_cast<const uint32_t*>(frame.get_data());
}

float RealsenseFrame::GetDepthUnits() {
    return frames.get_depth_frame().get_units();
}

void RealsenseFrame::ConvertDepth() {
    auto profile = frames.get_depth_frame();
    auto count = profile.get_width() * profile.get_height();
    converted_depth.resize(count * sizeof(float));

    auto frame = static_cast<const short*>(frames.get_depth_frame().get_data());

    for (int i = 0; i < count; i++) {
        converted_depth[i] = static_cast<float>(frame[i]);
    }
}

RealsenseFrame::~RealsenseFrame() = default;
