//
// Created by Kyle Smith on 2026-06-16.
//

#include "RemoveBlobsNode.h"
#include "../EmbeddedKernels.h"
#include "../PointmapperPipeline.h"

#include <algorithm>

namespace pointmapper::pipeline {
    RemoveBlobsNode::RemoveBlobsNode() {
        depthMap = CreateOutput<GPUDepthMap>();
        inputDepthMap = CreateInput<GPUDepthMap>();
        camera_params = CreateInput<CameraParams>();
        frameData = CreateInput<FrameData>();
    }

    void RemoveBlobsNode::Hydrate() {
        auto baseBlob = PIPELINE->GetComputePipelineByName("remove_blobs");
        if (baseBlob == nullptr) {
            std::vector<WGPUBindGroupLayoutEntry> group0Entries(1);
            group0Entries[0] = WGPU_BIND_GROUP_LAYOUT_ENTRY_INIT;
            group0Entries[0].buffer.type = WGPUBufferBindingType_Uniform;
            group0Entries[0].buffer.minBindingSize = sizeof(ComputePipelineInfo);
            group0Entries[0].visibility = WGPUShaderStage_Compute;

            std::vector<WGPUBindGroupLayoutEntry> group1Entries(1);
            group1Entries[0] = WGPU_BIND_GROUP_LAYOUT_ENTRY_INIT;
            group1Entries[0].binding = 0;
            group1Entries[0].buffer.type = WGPUBufferBindingType_Storage;
            group1Entries[0].visibility = WGPUShaderStage_Compute;

            std::vector<WGPUBindGroupLayoutDescriptor> layouts(2);
            layouts[0] = WGPU_BIND_GROUP_LAYOUT_DESCRIPTOR_INIT;
            layouts[0].entryCount = 1;
            layouts[0].entries = group0Entries.data();
            layouts[1] = WGPU_BIND_GROUP_LAYOUT_DESCRIPTOR_INIT;
            layouts[1].entryCount = 1;
            layouts[1].entries = group1Entries.data();

            auto kernels = PIPELINE->CompileShaderModule(embeddedKernels);
            baseBlob = PIPELINE->BuildComputePipeline("remove_blobs", kernels, "remove_blobs", std::span(layouts), 0);
        }
        blobPipeline = baseBlob->CreateInstance();

        pipelineInfoBuffer = PIPELINE->CreateBuffer(sizeof(ComputePipelineInfo), WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst);

        WGPUBindGroupEntry infoEntry = WGPU_BIND_GROUP_ENTRY_INIT;
        infoEntry.buffer = pipelineInfoBuffer;

        auto inputDepth = (*inputDepthMap).operator->();
        WGPUBindGroupEntry depthEntry = WGPU_BIND_GROUP_ENTRY_INIT;
        depthEntry.buffer = inputDepth->buffer;

        blobPipeline->SetBinding(0, infoEntry);
        blobPipeline->SetBinding(1, depthEntry);
        blobPipeline->CommitBindings();

        auto& out = *(*depthMap);
        out = *inputDepth;
        depthMap->MarkReady();
    }

    void RemoveBlobsNode::Process(PipelineBundle &bundle) {
        const auto& p = *(*camera_params).operator->();

        ComputePipelineInfo info = {
            .fx = p.fx,
            .fy = p.fy,
            .cx = p.cx,
            .cy = p.cy,
            .width = static_cast<uint32_t>(p.width),
            .height = static_cast<uint32_t>(p.height),
            .colorWidth = static_cast<uint32_t>(p.colorWidth),
            .colorHeight = static_cast<uint32_t>(p.colorHeight),
            .depth_tolerance = 0.5f
        };

        auto queue = PIPELINE->GetQueue();
        wgpuQueueWriteBuffer(queue, pipelineInfoBuffer, 0, &info, sizeof(info));

        int groupsX = std::max(1, static_cast<int>(p.width) / 8);
        int groupsY = std::max(1, static_cast<int>(p.height) / 8);

        blobPipeline->DispatchWorkgroups(bundle.encoder, groupsX, groupsY, 1, nullptr);
    }
}
