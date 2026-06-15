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

struct RegistrationInfo {
    mat4 transform;
};

class PointCloudComponent : public glengine::world::ActorSceneComponent {
public:
    PointCloudComponent();

    void Update(double deltaTime) override;
    void Render(const glengine::pipeline::wgpu::RenderBundle &, glengine::MatrixStack &stack) override;

    void SetDevice(DepthDevice* dev);
    void SetCloudNode(const std::shared_ptr<pointmapper::pipeline::CreatePointCloudNode>& node);

    void SetRegistration(mat4 transform);

private:
    std::shared_ptr<glengine::pipeline::wgpu::RenderPipeline> cloudRenderer;

    glengine::pipeline::wgpu::GPUPointer<IndirectRenderParams> indirectParams;
    glengine::pipeline::wgpu::GPUPointer<RegistrationInfo> registrationInfo;

    std::shared_ptr<pointmapper::pipeline::CreatePointCloudNode> cloudNode;

    std::shared_ptr<glengine::pipeline::wgpu::GPUMesh> mesh;
    DepthDevice *depthDevice;
};
