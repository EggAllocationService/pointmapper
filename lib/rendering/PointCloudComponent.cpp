//
// Created by Kyle Smith on 2026-06-02.
//

#include "PointCloudComponent.h"

#include "Engine.h"
#include "3d/mesh/StaticMesh.h"

PointCloudComponent::PointCloudComponent() {
    depthDevice = nullptr;
    indirectParams = GetEngine()->GetRenderer()->AllocateObject<IndirectRenderParams>(WGPUBufferUsage_Indirect);
    registrationInfo = GetEngine()->GetRenderer()->AllocateObject<RegistrationInfo>(WGPUBufferUsage_Uniform);

    registrationInfo->transform = mat4::identity();
    registrationInfo.Commit();

    cloudRenderer = GetEngine()->GetRenderer()->GetRenderPipelineByName("cloud")->CreateInstance();

    mesh = GetEngine()->GetResourceManager()->GetResource<glengine::world::mesh::StaticMesh>("/builtin/models/cube.obj")->mesh;

    WGPUBindGroupEntry regEntry = WGPU_BIND_GROUP_ENTRY_INIT;
    regEntry.binding = 1;
    regEntry.buffer = registrationInfo;
    cloudRenderer->SetBinding(1, regEntry);
}

void PointCloudComponent::Update(double deltaTime) {
    if (cloudNode == nullptr) return;

    auto renderer = GetEngine()->GetRenderer();
    auto device = renderer->GetDevice();
    auto queue = wgpuDeviceGetQueue(device);

    indirectParams->numIndices = mesh->GetIndexCount();
    indirectParams->numInstances = 0;
    indirectParams->firstVertex = 0;
    indirectParams->baseVertex = 0;
    indirectParams->firstInstance = 0;
    indirectParams.Commit();

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

    WGPUBindGroupEntry pointsEntry = WGPU_BIND_GROUP_ENTRY_INIT;
    pointsEntry.buffer = (*cloudNode)->points;

    cloudRenderer->SetBinding(1, pointsEntry);
    cloudRenderer->CommitBindings();
}

void PointCloudComponent::SetRegistration(mat4 transform) {
    registrationInfo->transform = transform;
    registrationInfo.Commit();
}
