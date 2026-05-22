//
// Created by Kyle Smith on 2026-05-13.
//
#pragma once
#include <vector>
#include <webgpu/wgpu.h>

#include "../common.h"

struct PipelineInfo {
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


class BackgroundProcessor {
public:
    BackgroundProcessor();

    void processFrame(std::shared_ptr<Frame>, std::vector<PointXYZRGB>& pointCloud);

    void resize(int depthWidth, int depthHeight, int colorWidth, int colorHeight, float fx, float fy, float cx, float cy);

    void resize(CameraParams params);

    /// copies depth to maxDepth
    void recalibrate();
private:
    void createPipelines();

    WGPUDevice device;
    WGPUQueue queue;

    WGPUBuffer depth;

    WGPUBuffer prevDepth;
    WGPUBuffer maxDepth;

    WGPUBuffer uniforms;

    WGPUBuffer output;
    WGPUBuffer outputInfo;
    WGPUBuffer outputCopy;

    WGPUTexture colorTexture;
    WGPUTextureView colorTextureView;
    WGPUSampler sampler;

    WGPUComputePipeline maskPipeline;
    WGPUComputePipeline cloudPipeline;

    WGPUShaderModule kernelModule;
    WGPUBindGroupLayout bindLayouts[4];

    WGPUBindGroup bindGroups[4];

    PipelineInfo info;
};