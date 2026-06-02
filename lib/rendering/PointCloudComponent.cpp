//
// Created by Kyle Smith on 2026-06-02.
//

#include "PointCloudComponent.h"

#include "Engine.h"
#include "../../cmake-build-release/_deps/glengine-src-src/include/3d/mesh/StaticMesh.h"

#define SPIN_IF(cond) if(cond) {std::this_thread::sleep_for(std::chrono::milliseconds(10)); continue;}

static void readingThread(PointCloudComponent *component) {
    while (true) {
        auto dev = component->GetDevice();
        SPIN_IF(dev == nullptr);

        auto frame = dev->GetNextFrame();
        SPIN_IF(frame == nullptr);

        component->Frame = frame.get();
        component->FrameWaiting = true;

        while (component->FrameWaiting) {
            SPIN_IF(true);
        }
    }
}

PointCloudComponent::PointCloudComponent() {
    depthDevice = nullptr;
    FrameWaiting = false;
    indirectParams = GetEngine()->GetRenderer()->AllocateObject<IndirectRenderParams>(WGPUBufferUsage_Storage | WGPUBufferUsage_Indirect);
    pipelineInfo = GetEngine()->GetRenderer()->AllocateObject<PipelineInfo>(WGPUBufferUsage_Uniform);

    maskPipeline = GetEngine()->GetRenderer()->GetComputePipelineByName("mask")->CreateInstance();
    cloudPipeline = GetEngine()->GetRenderer()->GetComputePipelineByName("cloud")->CreateInstance();

    cloudRenderer = GetEngine()->GetRenderer()->GetRenderPipelineByName("cloud")->CreateInstance();

    WGPUBindGroupEntry indirectEntry = WGPU_BIND_GROUP_ENTRY_INIT;
    indirectEntry.buffer = indirectParams;
    indirectEntry.binding = 1;

    WGPUBindGroupEntry piplineInfoEntry = WGPU_BIND_GROUP_ENTRY_INIT;
    piplineInfoEntry.buffer = pipelineInfo;

    maskPipeline->SetBinding(0, piplineInfoEntry);
    maskPipeline->SetBinding(0, indirectEntry);

    cloudPipeline->SetBinding(0, indirectEntry);
    cloudPipeline->SetBinding(0, piplineInfoEntry);

    maskPipeline->CommitBindings();
    cloudPipeline->CommitBindings();

    mesh = GetEngine()->GetResourceManager()->GetResource<glengine::world::mesh::StaticMesh>("/Users/kyle/CLionProjects/GLEngine/test3d/assets/cube-tex.obj")->mesh;
    readingThread = std::thread(::readingThread, this);
}

void PointCloudComponent::Update(double deltaTime) {
    if (depthDevice == nullptr || !FrameWaiting) return;
    const auto& p = *pipelineInfo;
    // write frame data
    auto frame = Frame;

    // reset mesh parameters
    indirectParams->numIndices = mesh->GetIndexCount();
    indirectParams->numInstances = 0;

    auto renderer = GetEngine()->GetRenderer();
    auto session = renderer->GetTransferManager()->CreateSession("Point cloud data transfer", 0);

    indirectParams.Commit(*session);
    session->Transfer(depthBuffer, 0, frame->GetDepth(), sizeof(float) * p.width * p.height);
    session->Commit(); // submit all uploads

    // perform the mask and cloud generation passes
    auto bundle = renderer->BeginComputePass();
    float scale = frame->GetDepthUnits();
    auto axisScalar = frame->GetAxisScale();
    maskPipeline->DispatchWorkgroups(bundle, p.width, p.height, 1, &scale);
    cloudPipeline->DispatchWorkgroups(bundle, p.width, p.height, 1, &axisScalar);
    renderer->CommitComputePass(bundle);

    FrameWaiting = false;
}

void PointCloudComponent::Render(const glengine::pipeline::wgpu::RenderBundle& bundle, glengine::MatrixStack &stack) {
    if (depthDevice == nullptr) return;

    mat4 transform = stack;
    cloudRenderer->DrawMeshInstancedIndirect(bundle, *mesh, indirectParams, &transform);
}

void PointCloudComponent::SetDevice(DepthDevice *dev) {
    depthDevice = dev;

    auto params = dev->GetCameraParameters();
    auto renderer = GetEngine()->GetRenderer();
    depthBuffer = renderer->CreateRawBuffer("Camera depth data", WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst, params.width * params.height * sizeof(float));
    pointsBuffer = renderer->CreateRawBuffer("Point cloud data", WGPUBufferUsage_Storage, params.width * params.height * sizeof(PointXYZRGB));
    previousDepthBuffer = renderer->CreateRawBuffer("Previous depth", WGPUBufferUsage_Storage, params.width * params.height * sizeof(float));
    maxDepthBuffer = renderer->CreateRawBuffer("Max depth", WGPUBufferUsage_Storage, params.width * params.height * sizeof(float));

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


    WGPUBindGroupEntry depthEntry = WGPU_BIND_GROUP_ENTRY_INIT;
    depthEntry.buffer = depthBuffer;
    WGPUBindGroupEntry pointsEntry = WGPU_BIND_GROUP_ENTRY_INIT;
    pointsEntry.binding = 1;
    pointsEntry.buffer = pointsBuffer;

    WGPUBindGroupEntry maxDepthEntry = WGPU_BIND_GROUP_ENTRY_INIT;
    maxDepthEntry.buffer = maxDepthBuffer;
    WGPUBindGroupEntry prevDepthEntry = WGPU_BIND_GROUP_ENTRY_INIT;
    prevDepthEntry.buffer = previousDepthBuffer;
    prevDepthEntry.binding = 1;

    maskPipeline->SetBinding(1, depthEntry);
    maskPipeline->SetBinding(1, pointsEntry);
    maskPipeline->SetBinding(2, maxDepthEntry);
    maskPipeline->SetBinding(2, prevDepthEntry);
    maskPipeline->CommitBindings();

    cloudPipeline->SetBinding(1, depthEntry);
    cloudPipeline->SetBinding(1, pointsEntry);
    cloudPipeline->SetBinding(2, maxDepthEntry);
    cloudPipeline->SetBinding(2, prevDepthEntry);
    cloudPipeline->CommitBindings();

    pointsEntry.binding = 0;
    cloudRenderer->SetBinding(1, pointsEntry);
    cloudRenderer->CommitBindings();
}
