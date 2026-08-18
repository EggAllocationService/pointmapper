//
// Created by Kyle Smith on 2026-06-02.
//

#include "pointmapper/rendering/PointCloudComponent.h"

#include "Engine.h"
#include "3d/mesh/StaticMesh.h"

PointCloudComponent::PointCloudComponent() {
    depthDevice = nullptr;
    indirectParams = GetEngine()->GetRenderer()->AllocateObject<IndirectRenderParams>(WGPUBufferUsage_Indirect);
    renderParams = GetEngine()->GetRenderer()->AllocateObject<RenderParams>(WGPUBufferUsage_Uniform);
    renderParams->tint = {0,0,0,0};

    cloudRenderer = GetEngine()->GetRenderer()->GetRenderPipelineByName("cloud")->CreateInstance();

    mesh = GetEngine()->GetResourceManager()->GetResource<glengine::world::mesh::StaticMesh>("/builtin/models/plane.obj")->mesh;

    cloudRenderer->SetBinding(1, 1, renderParams);

    indirectParams->numIndices = mesh->GetIndexCount();
    indirectParams->numInstances = 0;
    indirectParams->firstVertex = 0;
    indirectParams->baseVertex = 0;
    indirectParams->firstInstance = 0;
    indirectParams.Commit();
}

void PointCloudComponent::Update(double deltaTime) {
    if (cloudNode == nullptr) return;

    auto renderer = GetEngine()->GetRenderer();
    auto device = renderer->GetDevice();
    auto queue = wgpuDeviceGetQueue(device);

    renderParams.Commit();

    auto encoder = wgpuDeviceCreateCommandEncoder(device, nullptr);
    wgpuCommandEncoderCopyBufferToBuffer(
        encoder,
        (*cloudNode)->pointCount,
        sizeof(uint32_t),
        indirectParams,
        sizeof(uint32_t),
        sizeof(uint32_t));
    auto cmd = wgpuCommandEncoderFinish(encoder, nullptr);
    wgpuCommandEncoderRelease(encoder);
    wgpuQueueSubmit(queue, 1, &cmd);
    wgpuCommandBufferRelease(cmd);
}

void PointCloudComponent::Render(const glengine::pipeline::wgpu::RenderBundle& bundle, glengine::MatrixStack &stack) {
    if (cloudNode == nullptr) return;

    mat4 transform = stack;
    cloudRenderer->DrawMeshInstancedIndirect(bundle, *mesh, indirectParams, &transform);
}

void PointCloudComponent::SetDevice(DepthDevice *dev) {
    depthDevice = dev;
}

void PointCloudComponent::SetCloudNode(const std::shared_ptr<pointmapper::pipeline::Output<pointmapper::pipeline::GPUPointCloud>>& node) {
    cloudNode = node;

    cloudRenderer->SetBinding(1, 0, (*cloudNode)->points);
    cloudRenderer->CommitBindings();
}

void PointCloudComponent::SetTint(float4 tint) {
    renderParams->tint = tint;
}

void PointCloudComponent::SetMaxDepth(float maxDepth) {
    renderParams->tintDepth = maxDepth;
}
