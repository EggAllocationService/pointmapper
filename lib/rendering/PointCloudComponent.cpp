//
// Created by Kyle Smith on 2026-06-02.
//

#include "PointCloudComponent.h"

#include "Engine.h"

PointCloudComponent::PointCloudComponent() {
    depthDevice = nullptr;
    indirectParams = GetEngine()->GetRenderer()->AllocateObject<IndirectRenderParams>(WGPUBufferUsage_Storage | WGPUBufferUsage_Indirect);
    pipelineInfo = GetEngine()->GetRenderer()->AllocateObject<PipelineInfo>(WGPUBufferUsage_Uniform);

    maskPipeline = GetEngine()->GetRenderer()->GetComputePipelineByName("mask")->CreateInstance();
    cloudPipeline = GetEngine()->GetRenderer()->GetComputePipelineByName("cloud")->CreateInstance();

    cloudRenderer = GetEngine()->GetRenderer()->GetRenderPipelineByName("cloud")->CreateInstance();
}

void PointCloudComponent::Update(double deltaTime) {
    if (depthDevice == nullptr) return;

    // write frame data
    auto frame = depthDevice->GetNextFrame();
    memcpy(depthBuffer->GetData(), frame->GetDepth(), sizeof(float) * pipelineInfo->width * pipelineInfo->height);


    // reset mesh parameters
    indirectParams->numIndices = mesh->GetIndexCount();
    indirectParams->numInstances = 0;

    auto renderer = GetEngine()->GetRenderer();
    auto session = renderer->GetTransferManager()->CreateSession("Point cloud data transfer", 0);

    indirectParams.Commit(*session);
    depthBuffer->Commit(session, [](WGPUBuffer) {
        printf("Resize shouldn't have happened!!!");
        std::abort();
    });
    session->Commit(); // submit all uploads

    // perform the mask and cloud generation passes
    auto bundle = renderer->BeginComputePass();
    float scale = frame->GetDepthUnits();
    maskPipeline->DispatchWorkgroups(bundle, pipelineInfo->width, pipelineInfo->height, 0, &scale);
    cloudPipeline->DispatchWorkgroups(bundle, pipelineInfo->width, pipelineInfo->height, 0, nullptr);
    renderer->CommitComputePass(bundle);
}

void PointCloudComponent::Render(const glengine::pipeline::wgpu::RenderBundle& bundle, glengine::MatrixStack &stack) {
    if (depthDevice == nullptr) return;

    mat4 transform = stack;
    cloudRenderer->DrawMeshInstancedIndirect(bundle, *mesh, indirectParams, &stack);
}

void PointCloudComponent::SetDevice(DepthDevice *dev) {
    depthDevice = dev;

    auto params = dev->GetCameraParameters();
    depthBuffer = GetEngine()->GetRenderer()->CreateBuffer<float>("Camera depth data", WGPUBufferUsage_Storage, params.width * params.height);
    pointsBuffer = GetEngine()->GetRenderer()->CreateBuffer<PointXYZRGB>("Point cloud data", WGPUBufferUsage_Storage, params.width * params.height);

    pipelineInfo->cx = params.cx;
    pipelineInfo->cy = params.cy;
    pipelineInfo->colorHeight = params.colorHeight;
    pipelineInfo->colorWidth = params.colorWidth;
    pipelineInfo->depth_tolerance = 0.01;
    pipelineInfo->fx = params.fx;
    pipelineInfo->fy = params.fy;
    pipelineInfo->height = params.height;
    pipelineInfo->width = params.width;
    pipelineInfo.Commit();


}
