//
// Created by Kyle Smith on 2026-06-16.
//

#include "GpuToCpuCopyNode.h"
#include "../PointmapperPipeline.h"

#include <cstring>
#include <future>
#include <webgpu/webgpu.h>
#include <webgpu/wgpu.h>

namespace pointmapper::pipeline {
    static void MapCallback(WGPUMapAsyncStatus status, WGPUStringView, void* userdata1, void*) {
        auto* promise = static_cast<std::promise<bool>*>(userdata1);
        promise->set_value(status == WGPUMapAsyncStatus_Success);
    }

    GpuToCpuCopyNode::GpuToCpuCopyNode() {
        cloud = CreateInput<GPUPointCloud>();
        cpuCloud = CreateOutput<CPUPointCloud>();
    }

    GpuToCpuCopyNode::~GpuToCpuCopyNode() {
        if (stagingPoints != nullptr) {
            wgpuBufferRelease(stagingPoints);
        }
        if (stagingPointCount != nullptr) {
            wgpuBufferRelease(stagingPointCount);
        }
    }

    void GpuToCpuCopyNode::Hydrate() {
        auto input = (*cloud).operator->();
        pointCapacityBytes = static_cast<size_t>(input->maximumPointCount) * sizeof(PointXYZRGB);

        stagingPoints = PIPELINE->CreateBuffer(
            static_cast<unsigned int>(pointCapacityBytes),
            WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead);

        stagingPointCount = PIPELINE->CreateBuffer(
            16,
            WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead);

        (*cpuCloud)->maximumPointCount = input->maximumPointCount;
        (*cpuCloud)->points.reserve(input->maximumPointCount);
        cpuCloud->MarkReady();
    }

    void GpuToCpuCopyNode::Process(PipelineBundle& bundle) {
        auto input = (*cloud).operator->();

        // Copy commands must be recorded outside of a compute pass.
        bundle.EndComputePass();

        // Copy the GPU point data and the true point count into staging buffers.
        wgpuCommandEncoderCopyBufferToBuffer(bundle.cmd, input->pointCount, sizeof(uint32_t),
                                             stagingPointCount, 0, sizeof(uint32_t));
        wgpuCommandEncoderCopyBufferToBuffer(bundle.cmd, input->points, 0,
                                             stagingPoints, 0, pointCapacityBytes);

        // Submit the copies so the staging buffers are ready to be mapped.
        bundle.Flush();

        // Read back the actual number of points (instanceCount = second u32 of indirect info).
        ReadbackBuffer(stagingPointCount, sizeof(uint32_t));
        uint32_t pointCount = 0;
        {
            auto* mapped = static_cast<const uint8_t*>(wgpuBufferGetMappedRange(stagingPointCount, 0, sizeof(uint32_t)));
            if (mapped != nullptr) {
                std::memcpy(&pointCount, mapped, sizeof(uint32_t));
            }
            wgpuBufferUnmap(stagingPointCount);
        }

        auto& out = **cpuCloud;
        out.points.resize(pointCount);

        if (pointCount > 0) {
            ReadbackBuffer(stagingPoints, pointCapacityBytes);
            auto* mapped = wgpuBufferGetMappedRange(stagingPoints, 0, pointCapacityBytes);
            if (mapped != nullptr) {
                std::memcpy(out.points.data(), mapped, pointCount * sizeof(PointXYZRGB));
            }
            wgpuBufferUnmap(stagingPoints);
        }
    }

    void GpuToCpuCopyNode::ReadbackBuffer(WGPUBuffer buffer, size_t size) {
        std::promise<bool> promise;
        auto future = promise.get_future();

        WGPUBufferMapCallbackInfo cbInfo = WGPU_BUFFER_MAP_CALLBACK_INFO_INIT;
        cbInfo.mode = WGPUCallbackMode_WaitAnyOnly;
        cbInfo.callback = MapCallback;
        cbInfo.userdata1 = &promise;

        wgpuBufferMapAsync(buffer, WGPUMapMode_Read, 0, size, cbInfo);
        wgpuDevicePoll(PIPELINE->GetDevice(), true, nullptr);

        if (!future.get()) {
            printf("GpuToCpuCopyNode: failed to map staging buffer\n");
        }
    }
}
