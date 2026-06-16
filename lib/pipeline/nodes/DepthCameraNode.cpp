//
// Created by Kyle Smith on 2026-06-11.
//

#include "DepthCameraNode.h"
#include "../PointmapperPipeline.h"
#include "../../common.h"

#include <algorithm>
#include <cstring>

pointmapper::pipeline::DepthCameraNode::DepthCameraNode(DepthDevice* d) {
    depth = CreateOutput<GPUDepthMap>();
    params = CreateOutput<CameraParams>();
    color = CreateOutput<GPUColorTexture>();
    frameData = CreateOutput<GPUFrameData>();
    device = d;
}

pointmapper::pipeline::DepthCameraNode::~DepthCameraNode() {
    {
        std::lock_guard<std::mutex> lock(pendingMutex);
        stopThread = true;
    }
    pendingCV.notify_all();

    if (captureThread.joinable()) {
        captureThread.join();
    }
}

void pointmapper::pipeline::DepthCameraNode::Hydrate() {
    auto& p = **params;
    p = device->GetCameraParameters();
    cameraParams = p;

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

    captureThread = std::thread(&DepthCameraNode::CaptureThread, this);
}

void pointmapper::pipeline::DepthCameraNode::Process(PipelineBundle&) {
    PendingFrame frame;
    {
        std::lock_guard<std::mutex> lock(pendingMutex);
        if (!hasPendingFrame) {
            return;
        }
        frame = std::move(pendingFrame);
        hasPendingFrame = false;
    }

    pendingCV.notify_all();

    auto& d = **depth;
    auto queue = PIPELINE->GetQueue();

    wgpuQueueWriteBuffer(queue, d.buffer, 0, frame.depth.data(), frame.depth.size() * sizeof(float));

    auto copyInfo = WGPUTexelCopyTextureInfo {
        .texture = *(*color)->texture,
        .mipLevel = 0,
        .origin = { .x = 0, .y = 0, .z = 0 },
        .aspect = WGPUTextureAspect_All
    };
    auto copyBufLayout = WGPUTexelCopyBufferLayout {
        .offset = 0,
        .bytesPerRow = static_cast<uint32_t>(4 * cameraParams.colorWidth),
        .rowsPerImage = static_cast<uint32_t>(cameraParams.colorHeight),
    };
    auto extent = WGPUExtent3D {
        static_cast<uint32_t>(cameraParams.colorWidth),
        static_cast<uint32_t>(cameraParams.colorHeight),
        1
    };
    wgpuQueueWriteTexture(queue, &copyInfo, frame.color.data(),
        cameraParams.colorWidth * cameraParams.colorHeight * 4,
        &copyBufLayout, &extent);

    auto& fd = **frameData;
    fd.depthUnits = frame.depthUnits;
    fd.axisScale[0] = frame.axisScale[0];
    fd.axisScale[1] = frame.axisScale[1];
    fd.axisScale[2] = frame.axisScale[2];
    fd.axisScale[3] = frame.axisScale[3];

    frameData->NotifyAll();
    depth->NotifyAll();
    color->NotifyAll();
}

void pointmapper::pipeline::DepthCameraNode::CaptureThread() {
    const size_t depthPixels = static_cast<size_t>(cameraParams.width * cameraParams.height);
    const size_t colorPixels = static_cast<size_t>(cameraParams.colorWidth * cameraParams.colorHeight);

    while (true) {
        {
            std::unique_lock<std::mutex> lock(pendingMutex);
            if (stopThread) {
                return;
            }
        }

        auto currentFrame = device->GetNextFrame();
        if (currentFrame == nullptr) {
            continue;
        }

        PendingFrame frame;
        frame.depth.reserve(depthPixels);
        frame.color.reserve(colorPixels);

        const float* depthPtr = currentFrame->GetDepth();
        if (depthPtr != nullptr) {
            frame.depth.assign(depthPtr, depthPtr + depthPixels);
        }

        const uint32_t* colorPtr = currentFrame->GetColor();
        if (colorPtr != nullptr) {
            frame.color.assign(colorPtr, colorPtr + colorPixels);
        }

        frame.depthUnits = currentFrame->GetDepthUnits();
        auto axis = currentFrame->GetAxisScale();
        frame.axisScale[0] = axis.x;
        frame.axisScale[1] = axis.y;
        frame.axisScale[2] = axis.z;
        frame.axisScale[3] = axis.w;

        // Must drop the frame before the next call to GetNextFrame.
        currentFrame.reset();

        {
            std::lock_guard<std::mutex> lock(pendingMutex);
            pendingFrame = std::move(frame);
            hasPendingFrame = true;
        }
        pendingCV.notify_all();
    }
}
