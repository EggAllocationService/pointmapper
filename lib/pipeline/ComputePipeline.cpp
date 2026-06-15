//
// Created by Kyle Smith on 2026-06-15.
//

#include "ComputePipeline.h"

namespace pointmapper::pipeline {

    ComputePipeline::ComputePipeline(WGPUDevice device, WGPUComputePipeline pipeline,
                                     std::vector<WGPUBindGroupLayout> layouts, uint32_t immediateDataSize) {
        _device = device;
        _pipeline = pipeline;
        _immediateDataSize = immediateDataSize;
        _layouts = std::move(layouts);
        _dirty = std::vector(_layouts.size(), false);
        _groups = std::vector<WGPUBindGroup>(_layouts.size(), nullptr);
        _entries.resize(_layouts.size());
    }

    ComputePipeline::ComputePipeline(const ComputePipeline& other) {
        _device = other._device;
        _pipeline = other._pipeline;
        _immediateDataSize = other._immediateDataSize;
        _groups = other._groups;
        _entries = other._entries;
        _layouts = other._layouts;
        _dirty = other._dirty;

        wgpuComputePipelineAddRef(_pipeline);
        for (size_t i = 0; i < _groups.size(); i++) {
            if (_groups[i] != nullptr) {
                wgpuBindGroupAddRef(_groups[i]);
            }
            if (_layouts[i] != nullptr) {
                wgpuBindGroupLayoutAddRef(_layouts[i]);
            }
        }
    }

    ComputePipeline::~ComputePipeline() {
        for (size_t i = 0; i < _groups.size(); i++) {
            wgpuBindGroupRelease(_groups[i]);
            wgpuBindGroupLayoutRelease(_layouts[i]);
        }
        wgpuComputePipelineRelease(_pipeline);
    }

    void ComputePipeline::SetBinding(int group, WGPUBindGroupEntry entry) {
        auto& vector = _entries[group];
        if (vector.size() < entry.binding + 1) {
            vector.resize(entry.binding + 1);
        }
        vector[entry.binding] = entry;
        _dirty[group] = true;
    }

    void ComputePipeline::CommitBindings() {
        for (size_t i = 0; i < _groups.size(); i++) {
            if (!_dirty[i]) continue;

            if (_groups[i] != nullptr) {
                wgpuBindGroupRelease(_groups[i]);
            }

            WGPUBindGroupDescriptor desc = {
                .nextInChain = nullptr,
                .label = {},
                .layout = _layouts[i],
                .entryCount = _entries[i].size(),
                .entries = _entries[i].data(),
            };
            _groups[i] = wgpuDeviceCreateBindGroup(_device, &desc);

            _dirty[i] = false;
        }
    }

    void ComputePipeline::DispatchWorkgroups(WGPUComputePassEncoder encoder, int x, int y, int z,
                                             void* immediateData) const {
        wgpuComputePassEncoderSetPipeline(encoder, _pipeline);
        for (size_t i = 0; i < _groups.size(); i++) {
            wgpuComputePassEncoderSetBindGroup(encoder, i, _groups[i], 0, nullptr);
        }
        if (_immediateDataSize > 0) {
            wgpuComputePassEncoderSetImmediates(encoder, 0, _immediateDataSize, immediateData);
        }
        wgpuComputePassEncoderDispatchWorkgroups(encoder, x, y, z);
    }

    void ComputePipeline::DispatchWorkgroupsIndirect(WGPUComputePassEncoder encoder, WGPUBuffer indirectArgs, int offset,
                                                     void* immediateData) const {
        wgpuComputePassEncoderSetPipeline(encoder, _pipeline);
        for (size_t i = 0; i < _groups.size(); i++) {
            wgpuComputePassEncoderSetBindGroup(encoder, i, _groups[i], 0, nullptr);
        }
        if (_immediateDataSize > 0) {
            wgpuComputePassEncoderSetImmediates(encoder, 0, _immediateDataSize, immediateData);
        }
        wgpuComputePassEncoderDispatchWorkgroupsIndirect(encoder, indirectArgs, offset);
    }

    std::shared_ptr<ComputePipeline> ComputePipeline::CreateInstance() const {
        return std::make_shared<ComputePipeline>(*this);
    }

}
