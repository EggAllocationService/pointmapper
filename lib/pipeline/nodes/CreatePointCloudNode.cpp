//
// Created by Kyle Smith on 2026-06-11.
//

#include "CreatePointCloudNode.h"
#include "../EmbeddedKernels.h"
#include "../PointmapperPipeline.h"
#include "../../common.h"

#include <algorithm>

struct CloudPushConstants {
    float4 axisScale;
    float depthScale;
};

namespace pointmapper::pipeline {
    CreatePointCloudNode::CreatePointCloudNode() {
        cloud = CreateOutput<GPUPointCloud>();

        depth_map = CreateInput<GPUDepthMap>();
        camera_params = CreateInput<CameraParams>();
        color = CreateInput<GPUColorTexture>();
        frameData = CreateInput<GPUFrameData>();
    }

    void CreatePointCloudNode::Hydrate() {
        auto base = PIPELINE->GetComputePipelineByName("cloud");
        if (base == nullptr) {
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

            std::vector<WGPUBindGroupLayoutEntry> group1Entries(2);
            group1Entries[0] = WGPU_BIND_GROUP_LAYOUT_ENTRY_INIT;
            group1Entries[0].binding = 0;
            group1Entries[0].buffer.type = WGPUBufferBindingType_Storage;
            group1Entries[0].visibility = WGPUShaderStage_Compute;

            group1Entries[1] = WGPU_BIND_GROUP_LAYOUT_ENTRY_INIT;
            group1Entries[1].binding = 1;
            group1Entries[1].buffer.type = WGPUBufferBindingType_Storage;
            group1Entries[1].visibility = WGPUShaderStage_Compute;

            std::vector<WGPUBindGroupLayoutEntry> group3Entries(2);
            group3Entries[0] = WGPU_BIND_GROUP_LAYOUT_ENTRY_INIT;
            group3Entries[0].sampler.type = WGPUSamplerBindingType_Filtering;
            group3Entries[0].visibility = WGPUShaderStage_Compute;

            group3Entries[1] = WGPU_BIND_GROUP_LAYOUT_ENTRY_INIT;
            group3Entries[1].texture.sampleType = WGPUTextureSampleType_Float;
            group3Entries[1].texture.viewDimension = WGPUTextureViewDimension_2D;
            group3Entries[1].visibility = WGPUShaderStage_Compute;
            group3Entries[1].binding = 1;

            std::vector<WGPUBindGroupLayoutDescriptor> layouts(3);
            layouts[0] = WGPU_BIND_GROUP_LAYOUT_DESCRIPTOR_INIT;
            layouts[0].entryCount = 2;
            layouts[0].entries = group0Entries.data();
            layouts[1] = WGPU_BIND_GROUP_LAYOUT_DESCRIPTOR_INIT;
            layouts[1].entryCount = 2;
            layouts[1].entries = group1Entries.data();
            layouts[2] = WGPU_BIND_GROUP_LAYOUT_DESCRIPTOR_INIT;
            layouts[2].entryCount = 2;
            layouts[2].entries = group3Entries.data();

            auto kernels = PIPELINE->CompileShaderModule(embeddedKernels);
            base = PIPELINE->BuildComputePipeline("cloud", kernels, "create_cloud", std::span(layouts), sizeof(CloudPushConstants));
        }
        cloudPipeline = base->CreateInstance();

        const auto& p = *(*camera_params).operator->();
        auto depthPixels = p.width * p.height;
        auto pointBytes = static_cast<unsigned int>(depthPixels * sizeof(PointXYZRGB));

        auto& out = *(*cloud);
        out.points = PIPELINE->CreateBuffer(pointBytes, WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc);
        out.pointCount = PIPELINE->CreateBuffer(16, WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst);
        out.maximumPointCount = static_cast<unsigned int>(depthPixels);

        pipelineInfoBuffer = PIPELINE->CreateBuffer(sizeof(ComputePipelineInfo), WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst);
        sampler = wgpuDeviceCreateSampler(PIPELINE->GetDevice(), nullptr);

        WGPUBindGroupEntry infoEntry = WGPU_BIND_GROUP_ENTRY_INIT;
        infoEntry.buffer = pipelineInfoBuffer;

        WGPUBindGroupEntry countEntry = WGPU_BIND_GROUP_ENTRY_INIT;
        countEntry.buffer = out.pointCount;
        countEntry.binding = 1;

        auto inputDepth = (*depth_map).operator->();
        WGPUBindGroupEntry depthEntry = WGPU_BIND_GROUP_ENTRY_INIT;
        depthEntry.buffer = inputDepth->buffer;

        WGPUBindGroupEntry pointsEntry = WGPU_BIND_GROUP_ENTRY_INIT;
        pointsEntry.buffer = out.points;
        pointsEntry.binding = 1;

        WGPUBindGroupEntry samplerEntry = WGPU_BIND_GROUP_ENTRY_INIT;
        samplerEntry.sampler = sampler;

        WGPUBindGroupEntry colorEntry = WGPU_BIND_GROUP_ENTRY_INIT;
        colorEntry.textureView = *(*color)->texture;
        colorEntry.binding = 1;

        cloudPipeline->SetBinding(0, infoEntry);
        cloudPipeline->SetBinding(0, countEntry);
        cloudPipeline->SetBinding(1, depthEntry);
        cloudPipeline->SetBinding(1, pointsEntry);
        cloudPipeline->SetBinding(2, samplerEntry);
        cloudPipeline->SetBinding(2, colorEntry);
        cloudPipeline->CommitBindings();

        cloud->MarkReady();
    }

    void CreatePointCloudNode::Process(PipelineBundle& bundle) {
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

        uint32_t zeros[4] = {0, 0, 0, 0};
        wgpuQueueWriteBuffer(queue, (*cloud)->pointCount, 0, zeros, sizeof(zeros));

        auto fd = (*frameData).operator->();
        CloudPushConstants pc;
        pc.axisScale.x = fd->axisScale[0];
        pc.axisScale.y = fd->axisScale[1];
        pc.axisScale.z = fd->axisScale[2];
        pc.axisScale.w = fd->axisScale[3];
        pc.depthScale = fd->depthUnits;

        int groupsX = std::max(1, static_cast<int>(p.width) / 8);
        int groupsY = std::max(1, static_cast<int>(p.height) / 8);

        cloudPipeline->DispatchWorkgroups(bundle.encoder, groupsX, groupsY, 1, &pc);
    }
}
