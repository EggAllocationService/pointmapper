//
// Created by Kyle Smith on 2026-06-19.
//

#include "pointmapper/pipeline/nodes/CpuToGpuCopyNode.h"

#include "pointmapper/pipeline/PointmapperPipeline.h"

pointmapper::pipeline::CpuToGpuCopyNode::CpuToGpuCopyNode() {
    input = CreateInput<CPUPointCloud>();
    output = CreateOutput<GPUPointCloud>();
}

void pointmapper::pipeline::CpuToGpuCopyNode::Hydrate() {
    auto max = (*input)->maximumPointCount;
    (*output)->maximumPointCount = max;

    (*output)->points = PIPELINE->CreateBuffer(sizeof(PointXYZRGB) * max, WGPUBufferUsage_CopyDst | WGPUBufferUsage_CopySrc | WGPUBufferUsage_Storage);
    (*output)->pointCount = PIPELINE->CreateBuffer(16, WGPUBufferUsage_CopyDst | WGPUBufferUsage_Storage | WGPUBufferUsage_Indirect | WGPUBufferUsage_CopySrc);
}

void pointmapper::pipeline::CpuToGpuCopyNode::Process(PipelineBundle &) {
    auto& t = (*input);
    auto len = t->points.size();

    wgpuQueueWriteBuffer(PIPELINE->GetQueue(), (*output)->points, 0, t->points.data(), t->points.size() * sizeof(PointXYZRGB));
    wgpuQueueWriteBuffer(PIPELINE->GetQueue(), (*output)->pointCount, sizeof(uint32_t), &len, sizeof(uint32_t));
}
