//
// Created by Kyle Smith on 2026-06-11.
//

#include "pointmapper/pipeline/nodes/RemoveBackgroundNode.h"
#include "pointmapper/pipeline/EmbeddedKernels.h"
#include "pointmapper/pipeline/PointmapperPipeline.h"

#include <algorithm>

pointmapper::pipeline::RemoveBackgroundNode::RemoveBackgroundNode() {
    depthMap = CreateOutput<GPUDepthMap>();
    inputDepthMap = CreateInput<GPUDepthMap>();
    camera_params = CreateInput<CameraParams>();
    frameData = CreateInput<FrameData>();
}

void pointmapper::pipeline::RemoveBackgroundNode::Hydrate() {
    auto baseMask = PIPELINE->GetComputePipelineByName("mask");
    if (baseMask == nullptr) {
        std::vector<WGPUBindGroupLayoutEntry> group0Entries(2);
        group0Entries[0] = WGPU_BIND_GROUP_LAYOUT_ENTRY_INIT;
        group0Entries[0].buffer.type = WGPUBufferBindingType_Uniform;
        group0Entries[0].buffer.minBindingSize = sizeof(ComputePipelineInfo);
        group0Entries[0].visibility = WGPUShaderStage_Compute;

        group0Entries[1] = WGPU_BIND_GROUP_LAYOUT_ENTRY_INIT;
        group0Entries[1].buffer.type = WGPUBufferBindingType_Storage;
        group0Entries[1].buffer.minBindingSize = 16;
        group0Entries[1].visibility = WGPUShaderStage_Compute;
        group0Entries[1].binding = 1;

        std::vector<WGPUBindGroupLayoutEntry> group1Entries(3);
        group1Entries[0] = WGPU_BIND_GROUP_LAYOUT_ENTRY_INIT;
        group1Entries[0].binding = 0;
        group1Entries[0].buffer.type = WGPUBufferBindingType_Storage;
        group1Entries[0].visibility = WGPUShaderStage_Compute;

        group1Entries[1] = WGPU_BIND_GROUP_LAYOUT_ENTRY_INIT;
        group1Entries[1].binding = 1;
        group1Entries[1].buffer.type = WGPUBufferBindingType_Storage;
        group1Entries[1].visibility = WGPUShaderStage_Compute;

        group1Entries[2] = WGPU_BIND_GROUP_LAYOUT_ENTRY_INIT;
        group1Entries[2].binding = 2;
        group1Entries[2].buffer.type = WGPUBufferBindingType_Storage;
        group1Entries[2].visibility = WGPUShaderStage_Compute;

        std::vector<WGPUBindGroupLayoutDescriptor> layouts(3);
        layouts[0] = WGPU_BIND_GROUP_LAYOUT_DESCRIPTOR_INIT;
        layouts[0].entryCount = 2;
        layouts[0].entries = group0Entries.data();
        layouts[1] = WGPU_BIND_GROUP_LAYOUT_DESCRIPTOR_INIT;
        layouts[1].entryCount = 2;
        layouts[1].entries = group1Entries.data();
        layouts[2] = layouts[1];
        layouts[2].entryCount = 3;

        auto kernels = PIPELINE->CompileShaderModule(embeddedKernels);
        baseMask = PIPELINE->BuildComputePipeline("mask", kernels, "mask", std::span(layouts), 0);
    }
    maskPipeline = baseMask->CreateInstance();

    const auto& p = *(*camera_params).operator->();
    auto depthSize = static_cast<unsigned int>(p.width * p.height * sizeof(float));

    pipelineInfoBuffer = PIPELINE->CreateBuffer(sizeof(ComputePipelineInfo), WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst);
    dummyOutInfo = PIPELINE->CreateBuffer(16, WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst);
    maxDepth = PIPELINE->CreateBuffer(depthSize, WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst);
    prevDepth = PIPELINE->CreateBuffer(depthSize, WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst);
    unresolvedDepth = PIPELINE->CreateBuffer(depthSize, WGPUBufferUsage_Storage);

    uint32_t zeros[4] = {0, 0, 0, 0};
    wgpuQueueWriteBuffer(PIPELINE->GetQueue(), dummyOutInfo, 0, zeros, sizeof(zeros));

    WGPUBindGroupEntry infoEntry = WGPU_BIND_GROUP_ENTRY_INIT;
    infoEntry.buffer = pipelineInfoBuffer;

    WGPUBindGroupEntry outInfoEntry = WGPU_BIND_GROUP_ENTRY_INIT;
    outInfoEntry.buffer = dummyOutInfo;
    outInfoEntry.binding = 1;

    auto inputDepth = (*inputDepthMap).operator->();
    WGPUBindGroupEntry depthEntry = WGPU_BIND_GROUP_ENTRY_INIT;
    depthEntry.buffer = inputDepth->buffer;

    WGPUBindGroupEntry dummyPointsEntry = WGPU_BIND_GROUP_ENTRY_INIT;
    dummyPointsEntry.buffer = dummyOutInfo;
    dummyPointsEntry.binding = 1;

    maskPipeline->SetBinding(0, infoEntry);
    maskPipeline->SetBinding(0, outInfoEntry);
    maskPipeline->SetBinding(1, depthEntry);
    maskPipeline->SetBinding(1, dummyPointsEntry);

    WGPUBindGroupEntry maxDepthEntry = WGPU_BIND_GROUP_ENTRY_INIT;
    maxDepthEntry.buffer = maxDepth;
    WGPUBindGroupEntry prevDepthEntry = WGPU_BIND_GROUP_ENTRY_INIT;
    prevDepthEntry.buffer = prevDepth;
    prevDepthEntry.binding = 1;
    WGPUBindGroupEntry unresolvedDepthEntry = WGPU_BIND_GROUP_ENTRY_INIT;
    unresolvedDepthEntry.buffer = unresolvedDepth;
    unresolvedDepthEntry.binding = 2;
    maskPipeline->SetBinding(2, maxDepthEntry);
    maskPipeline->SetBinding(2, prevDepthEntry);
    maskPipeline->SetBinding(2, unresolvedDepthEntry);
    maskPipeline->CommitBindings();

    auto& out = *(*depthMap);
    out = *inputDepth;
    depthMap->MarkReady();
}

void pointmapper::pipeline::RemoveBackgroundNode::Process(PipelineBundle &bundle) {
    const auto& p = *(*camera_params).operator->();
    auto fd = (*frameData).operator->();
    float scale = fd->depthUnits;

    ComputePipelineInfo info = {
        .fx = p.fx,
        .fy = p.fy,
        .cx = p.cx,
        .cy = p.cy,
        .width = static_cast<uint32_t>(p.width),
        .height = static_cast<uint32_t>(p.height),
        .colorWidth = static_cast<uint32_t>(p.colorWidth),
        .colorHeight = static_cast<uint32_t>(p.colorHeight),
        .depth_tolerance = 0.5f / scale
    };

    auto queue = PIPELINE->GetQueue();
    wgpuQueueWriteBuffer(queue, pipelineInfoBuffer, 0, &info, sizeof(info));

    int groupsX = std::max(1, static_cast<int>(p.width) / 8);
    int groupsY = std::max(1, static_cast<int>(p.height) / 8);

    maskPipeline->DispatchWorkgroups(bundle.encoder, groupsX, groupsY, 1, nullptr);
}
