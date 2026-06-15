//
// Created by Kyle Smith on 2026-06-11.
//

#include "DepthCameraNode.h"
#include "../PointmapperPipeline.h"
#include "../../common.h"

pointmapper::pipeline::DepthCameraNode::DepthCameraNode(DepthDevice* d) {
    depth = CreateOutput<GPUDepthMap>();
    params = CreateOutput<CameraParams>();
    color = CreateOutput<GPUColorTexture>();
    frameData = CreateOutput<GPUFrameData>();
    device = d;
}

void pointmapper::pipeline::DepthCameraNode::Hydrate() {
    auto& p = **params;
    p = device->GetCameraParameters();

    auto& outDepth = **depth;
    outDepth.width = static_cast<unsigned int>(p.width);
    outDepth.height = static_cast<unsigned int>(p.height);
    outDepth.buffer = PIPELINE->CreateBuffer(
        p.width * p.height * sizeof(float),
        WGPUBufferUsage_CopyDst | WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc);

    auto& outColor = **color;
    outColor.texture = PIPELINE->CreateTexture(
        "Camera color",
        WGPUTextureUsage_CopyDst | WGPUTextureUsage_TextureBinding,
        p.colorType == RGBX ? WGPUTextureFormat_RGBA8Unorm : WGPUTextureFormat_BGRA8Unorm,
        static_cast<unsigned int>(p.colorWidth),
        static_cast<unsigned int>(p.colorHeight));

    params->MarkReady();
    depth->MarkReady();
    color->MarkReady();
    frameData->MarkReady();
}

void pointmapper::pipeline::DepthCameraNode::Process(PipelineBundle&) {
    currentFrame = device->GetNextFrame();
    if (currentFrame == nullptr) return;

    const auto& p = **params;
    auto& d = **depth;
    auto queue = PIPELINE->GetQueue();

    wgpuQueueWriteBuffer(queue, d.buffer, 0, currentFrame->GetDepth(), p.width * p.height * sizeof(float));

    auto copyInfo = WGPUTexelCopyTextureInfo {
        .texture = *(*color)->texture,
        .mipLevel = 0,
        .origin = { .x = 0, .y = 0, .z = 0 },
        .aspect = WGPUTextureAspect_All
    };
    auto copyBufLayout = WGPUTexelCopyBufferLayout {
        .offset = 0,
        .bytesPerRow = static_cast<uint32_t>(4 * p.colorWidth),
        .rowsPerImage = static_cast<uint32_t>(p.colorHeight),
    };
    auto extent = WGPUExtent3D {
        static_cast<uint32_t>(p.colorWidth),
        static_cast<uint32_t>(p.colorHeight),
        1
    };
    wgpuQueueWriteTexture(queue, &copyInfo, currentFrame->GetColor(), p.colorWidth * p.colorHeight * 4, &copyBufLayout, &extent);

    auto& fd = **frameData;
    fd.depthUnits = currentFrame->GetDepthUnits();
    auto scale = currentFrame->GetAxisScale();
    fd.axisScale[0] = scale.x;
    fd.axisScale[1] = scale.y;
    fd.axisScale[2] = scale.z;
    fd.axisScale[3] = scale.w;
}
