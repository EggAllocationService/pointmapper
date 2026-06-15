//
// Created by Kyle Smith on 2026-06-11.
//

#pragma once

#include "../GPUTexture.h"
#include "webgpu/wgpu.h"

#include <cstdint>
#include <memory>

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

    struct GPUColorTexture {
        std::shared_ptr<GPUTexture> texture;
    };

    struct GPUFrameData {
        float depthUnits;
        float axisScale[4];
    };

    struct ComputePipelineInfo {
        float fx;
        float fy;
        float cx;
        float cy;
        uint32_t width;
        uint32_t height;
        uint32_t colorWidth;
        uint32_t colorHeight;
        float depth_tolerance;
    };
}
