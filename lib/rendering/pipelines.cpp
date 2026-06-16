//
// Created by Kyle Smith on 2026-06-02.
//

#include "pipelines.h"
#include "PointResources.h"
#include "PointCloudComponent.h"


void addPointmapperPipelines(glengine::pipeline::wgpu::WGPURenderer* renderer) {
    auto shaders = renderer->CompileShader(embed_shaders_wgsl);

    WGPUBindGroupLayoutEntry renderEntries[2] = {WGPU_BIND_GROUP_LAYOUT_ENTRY_INIT, WGPU_BIND_GROUP_LAYOUT_ENTRY_INIT};
    renderEntries[0].buffer = {
        .nextInChain = nullptr,
        .type = WGPUBufferBindingType_ReadOnlyStorage,
        .hasDynamicOffset = false,
        .minBindingSize = 0
    };
    renderEntries[0].visibility = WGPUShaderStage_Vertex;

    renderEntries[1].buffer = {
        .nextInChain = nullptr,
        .type = WGPUBufferBindingType_Uniform,
        .hasDynamicOffset = false,
        .minBindingSize = sizeof(mat4)
    };
    renderEntries[1].visibility = WGPUShaderStage_Vertex;
    renderEntries[1].binding = 1;

    WGPUBindGroupLayoutDescriptor renderLayout = {
        .nextInChain = nullptr,
        .label = {},
        .entryCount = 2,
        .entries = &renderEntries[0]
    };

    renderer->BuildRenderPipeline("cloud", shaders, nullptr, std::span(&renderLayout, 1), sizeof(mat4), nullptr);
}
