//
// Created by Kyle Smith on 2026-06-02.
//

#include "PointCloudComponent.h"

#include "Engine.h"
#include "3d/mesh/StaticMesh.h"

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
    registrationInfo = GetEngine()->GetRenderer()->AllocateObject<RegistrationInfo>(WGPUBufferUsage_Uniform);

    registrationInfo->transform = mat4::identity();
    registrationInfo.Commit();

    maskPipeline = GetEngine()->GetRenderer()->GetComputePipelineByName("mask")->CreateInstance();
    cloudPipeline = GetEngine()->GetRenderer()->GetComputePipelineByName("cloud")->CreateInstance();
    blobPipeline = GetEngine()->GetRenderer()->GetComputePipelineByName("remove_blobs")->CreateInstance();

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

    blobPipeline->SetBinding(0, piplineInfoEntry);

    maskPipeline->CommitBindings();
    cloudPipeline->CommitBindings();

    mesh = GetEngine()->GetResourceManager()->GetResource<glengine::world::mesh::StaticMesh>("/builtin/models/cube.obj")->mesh;
    readingThread = std::thread(::readingThread, this);

    WGPUBindGroupEntry sampler = WGPU_BIND_GROUP_ENTRY_INIT;
    sampler.sampler = wgpuDeviceCreateSampler(GetEngine()->GetRenderer()->GetDevice(), nullptr);

    cloudPipeline->SetBinding(3, sampler);
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
    auto queue = wgpuDeviceGetQueue(renderer->GetDevice());
    auto copyInfo = WGPUTexelCopyTextureInfo {
        .texture = *colorTexture,
        .mipLevel = 0,
        .origin = {.x = 0, .y = 0, .z = 0},
        .aspect = WGPUTextureAspect_All
    };
    auto copyBufLayout = WGPUTexelCopyBufferLayout {
        .offset = 0,
        .bytesPerRow = 4 * p.colorWidth,
        .rowsPerImage = p.colorHeight,
    };

    auto extent = WGPUExtent3D {p.colorWidth, p.colorHeight, 1};
    wgpuQueueWriteTexture(queue, &copyInfo, frame->GetColor(), p.colorWidth * p.colorHeight * 4, &copyBufLayout, &extent);

    auto session = renderer->GetTransferManager()->CreateSession("Point cloud data transfer", 0);
    indirectParams.Commit(*session);
    session->Transfer(depthBuffer, 0, frame->GetDepth(), sizeof(float) * p.width * p.height);
    session->Commit(); // submit all uploads

    // perform the mask and cloud generation passes
    auto bundle = renderer->BeginComputePass();
    float scale = frame->GetDepthUnits();
    auto axisScalar = frame->GetAxisScale();
    maskPipeline->DispatchWorkgroups(bundle, p.width / 8, p.height / 8, 1, &scale);
    blobPipeline->DispatchWorkgroups(bundle, p.width / 8, p.height / 8, 1, nullptr);
    cloudPipeline->DispatchWorkgroups(bundle, p.width / 8, p.height / 8, 1, &axisScalar);
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
    colorTexture = renderer->CreateTexture(
        "Camera color data",
        WGPUTextureUsage_CopyDst | WGPUTextureUsage_TextureBinding,
        params.colorType == RGBX ? WGPUTextureFormat_RGBA8Unorm : WGPUTextureFormat_BGRA8Unorm,
        params.colorWidth,
        params.colorHeight
    );

    depthBuffer = renderer->CreateRawBuffer("Camera depth data", WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst, params.width * params.height * sizeof(float));
    pointsBuffer = renderer->CreateRawBuffer("Point cloud data", WGPUBufferUsage_Storage, params.width * params.height * sizeof(PointXYZRGB));
    previousDepthBuffer = renderer->CreateRawBuffer("Previous depth", WGPUBufferUsage_Storage, params.width * params.height * sizeof(float));
    maxDepthBuffer = renderer->CreateRawBuffer("Max depth", WGPUBufferUsage_Storage, params.width * params.height * sizeof(float));

    pipelineInfo->cx = params.cx;
    pipelineInfo->cy = params.cy;
    pipelineInfo->colorHeight = params.colorHeight;
    pipelineInfo->colorWidth = params.colorWidth;
    pipelineInfo->depth_tolerance = 0.5;
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

    WGPUBindGroupEntry colorEntry = WGPU_BIND_GROUP_ENTRY_INIT;
    colorEntry.binding = 1;
    colorEntry.textureView = *colorTexture;

    cloudPipeline->SetBinding(1, depthEntry);
    cloudPipeline->SetBinding(1, pointsEntry);
    cloudPipeline->SetBinding(2, maxDepthEntry);
    cloudPipeline->SetBinding(2, prevDepthEntry);
    cloudPipeline->SetBinding(3, colorEntry);
    cloudPipeline->CommitBindings();

    blobPipeline->SetBinding(1, depthEntry);
    blobPipeline->CommitBindings();

    pointsEntry.binding = 0;

    WGPUBindGroupEntry regEntry = WGPU_BIND_GROUP_ENTRY_INIT;
    regEntry.binding = 1;
    regEntry.buffer = registrationInfo;
    cloudRenderer->SetBinding(1, pointsEntry);
    cloudRenderer->SetBinding(1, regEntry);
    cloudRenderer->CommitBindings();
}

void PointCloudComponent::SetRegistration(mat4 transform) {
    registrationInfo->transform = transform;
    registrationInfo.Commit();
}
