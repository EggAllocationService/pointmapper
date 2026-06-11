//
// Created by Kyle Smith on 2026-06-11.
//

#pragma once

#include "wgpu.h"

namespace pointmapper::pipeline {
    struct GPUDepthMap {
        WGPUBuffer buffer;
        unsigned int width;
        unsigned int height;
    };

    /// points contains the actual point data
    /// pointCount is a single u32 indicating how many points there actually are
    /// maximumPointCount is the max capacity of `points`
    struct GPUPointCloud {
        WGPUBuffer points;
        WGPUBuffer pointCount;
        unsigned int maximumPointCount;
    };

    struct GPUMask {
        WGPUBuffer mask;
        unsigned int width;
        unsigned int height;
    };
}
