//
// Created by Kyle Smith on 2026-05-21.
//

#pragma once
#include <librealsense2/hpp/rs_frame.hpp>

#include "../common.h"


class RealsenseFrame : public Frame {
public:
    ~RealsenseFrame() override;
    const float *GetDepth() override;
    const uint32_t *GetColor() override;

    float GetDepthUnits() override;
    rs2::frameset frames;
};
