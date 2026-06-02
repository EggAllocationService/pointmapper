//
// Created by Kyle Smith on 2026-06-02.
//
#pragma once
#include "../DepthDevice.h"
#include "3d/ActorSceneComponent.h"
#include "pipeline/wgpu/WGPURenderer.h"

struct IndirectRenderParams {
    int numIndices;
    int numInstances;
    int firstVertex;
    int baseVertex;
    int firstInstance;
};

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

class PointCloudComponent : public glengine::world::ActorSceneComponent {
public:
    PointCloudComponent();

    void Update(double deltaTime) override;
    void Render(const glengine::pipeline::wgpu::RenderBundle &, glengine::MatrixStack &stack) override;

    void SetDevice(DepthDevice* dev);

private:
    std::shared_ptr<glengine::pipeline::wgpu::ComputePipeline> maskPipeline;
    std::shared_ptr<glengine::pipeline::wgpu::ComputePipeline> cloudPipeline;
    std::shared_ptr<glengine::pipeline::wgpu::RenderPipeline> cloudRenderer;

    std::shared_ptr<glengine::pipeline::wgpu::TypedGPUBuffer<float>> depthBuffer;
    std::shared_ptr<glengine::pipeline::wgpu::TypedGPUBuffer<PointXYZRGB>> pointsBuffer;
    glengine::pipeline::wgpu::GPUPointer<IndirectRenderParams> indirectParams;
    glengine::pipeline::wgpu::GPUPointer<PipelineInfo> pipelineInfo;

    std::shared_ptr<glengine::pipeline::wgpu::GPUMesh> mesh;
    DepthDevice *depthDevice;
};
