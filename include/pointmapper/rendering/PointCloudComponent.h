//
// Created by Kyle Smith on 2026-06-02.
//
#pragma once
#include "../DepthDevice.h"
#include "../pipeline/nodes/CreatePointCloudNode.h"
#include "3d/ActorSceneComponent.h"
#include "pipeline/wgpu/WGPURenderer.h"
#include <thread>

struct IndirectRenderParams {
    int numIndices;
    int numInstances;
    int firstVertex;
    int baseVertex;
    int firstInstance;
};

struct alignas(16) RenderParams {
    float4 tint;
    float tintDepth = 0;
};

class PointCloudComponent : public glengine::world::ActorSceneComponent {
public:
    PointCloudComponent();

    void Update(double deltaTime) override;
    void Render(const glengine::pipeline::wgpu::RenderBundle &, glengine::MatrixStack &stack) override;

    void SetDevice(DepthDevice* dev);
    void SetCloudNode(const std::shared_ptr<pointmapper::pipeline::Output<pointmapper::pipeline::GPUPointCloud>>& node);

    void SetTint(float4 tint);
    void SetMaxDepth(float maxDepth);

private:
    std::shared_ptr<glengine::pipeline::wgpu::RenderPipeline> cloudRenderer;

    glengine::pipeline::wgpu::GPUPointer<IndirectRenderParams> indirectParams;
    glengine::pipeline::wgpu::GPUPointer<RenderParams> renderParams;

    std::shared_ptr<pointmapper::pipeline::Output<pointmapper::pipeline::GPUPointCloud>> cloudNode;

    std::shared_ptr<glengine::pipeline::wgpu::GPUMesh> mesh;
    DepthDevice *depthDevice;
};
