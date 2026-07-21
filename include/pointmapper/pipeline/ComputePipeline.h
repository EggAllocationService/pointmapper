//
// Created by Kyle Smith on 2026-06-15.
//
#pragma once

#include <memory>
#include <vector>
#include <webgpu/wgpu.h>

namespace pointmapper::pipeline {

    class ComputePipeline {
    public:
        ComputePipeline(WGPUDevice device, WGPUComputePipeline pipeline,
                        std::vector<WGPUBindGroupLayout> layouts, uint32_t immediateDataSize);
        ComputePipeline(const ComputePipeline& other);
        ~ComputePipeline();

        void SetBinding(int group, WGPUBindGroupEntry entry);
        void CommitBindings();

        void DispatchWorkgroups(WGPUComputePassEncoder encoder, int x, int y, int z, void* immediateData) const;
        void DispatchWorkgroupsIndirect(WGPUComputePassEncoder encoder, WGPUBuffer indirectArgs, int offset,
                                        void* immediateData) const;

        [[nodiscard]] std::shared_ptr<ComputePipeline> CreateInstance() const;

    private:
        WGPUDevice _device;
        WGPUComputePipeline _pipeline;
        uint32_t _immediateDataSize;

        std::vector<WGPUBindGroupLayout> _layouts;
        std::vector<WGPUBindGroup> _groups;
        std::vector<std::vector<WGPUBindGroupEntry>> _entries;
        std::vector<bool> _dirty;
    };

}
