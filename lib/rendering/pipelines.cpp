//
// Created by Kyle Smith on 2026-06-02.
//

#include "pipelines.h"
#include "PointResources.h"
#include "PointCloudComponent.h"

void addPointmapperPipelines(glengine::pipeline::wgpu::WGPURenderer* renderer) {
    auto shaders = renderer->CompileShader(embed_shaders_wgsl);
    auto kernels = renderer->CompileShader(embed_kernels_wgsl);

    auto groupLayouts = new WGPUBindGroupLayoutDescriptor[4];
    auto group0Bindings = new WGPUBindGroupLayoutEntry[2] { WGPU_BIND_GROUP_LAYOUT_ENTRY_INIT, WGPU_BIND_GROUP_LAYOUT_ENTRY_INIT };
    group0Bindings[0].buffer.type = WGPUBufferBindingType_Uniform;
    group0Bindings[0].buffer.minBindingSize = sizeof(PipelineInfo);
    group0Bindings[0].visibility = WGPUShaderStage_Compute;

    group0Bindings[1].buffer.type = WGPUBufferBindingType_Storage;
    group0Bindings[1].buffer.minBindingSize = 16;
    group0Bindings[1].visibility = WGPUShaderStage_Compute;
    group0Bindings[1].binding = 1;

    groupLayouts[0] = WGPU_BIND_GROUP_LAYOUT_DESCRIPTOR_INIT;
    groupLayouts[0].entries = &group0Bindings[0];
    groupLayouts[0].entryCount = 2;

    // group 1 - depth i/o
    auto group1Bindings = new WGPUBindGroupLayoutEntry[2];
    group1Bindings[0] = WGPU_BIND_GROUP_LAYOUT_ENTRY_INIT;
    group1Bindings[0].binding = 0;
    group1Bindings[0].buffer.type = WGPUBufferBindingType_Storage;
    group1Bindings[0].visibility = WGPUShaderStage_Compute;

    group1Bindings[1] = WGPU_BIND_GROUP_LAYOUT_ENTRY_INIT;
    group1Bindings[1].binding = 1;
    group1Bindings[1].buffer.type = WGPUBufferBindingType_Storage;
    group1Bindings[1].visibility = WGPUShaderStage_Compute;

    groupLayouts[1] = WGPU_BIND_GROUP_LAYOUT_DESCRIPTOR_INIT;
    groupLayouts[1].entries = &group1Bindings[0];
    groupLayouts[1].entryCount = 2;

    // group 2 - mask internals
    groupLayouts[2] = groupLayouts[1];

    // group 3 - cloud generation textures
    auto group3Bindings = new WGPUBindGroupLayoutEntry[2] { WGPU_BIND_GROUP_LAYOUT_ENTRY_INIT, WGPU_BIND_GROUP_LAYOUT_ENTRY_INIT };
    group3Bindings[0].sampler.type = WGPUSamplerBindingType_Filtering;
    group3Bindings[0].visibility = WGPUShaderStage_Compute;

    group3Bindings[1].texture.sampleType = WGPUTextureSampleType_Float;
    group3Bindings[1].texture.viewDimension = WGPUTextureViewDimension_2D;
    group3Bindings[1].visibility = WGPUShaderStage_Compute;
    group3Bindings[1].binding = 1;

    groupLayouts[3].entries = &group3Bindings[0];
    groupLayouts[3].entryCount = 2;

    renderer->BuildComputePipeline("mask", kernels, "mask", std::span(groupLayouts, 3), sizeof(float));
    renderer->BuildComputePipeline("cloud", kernels, "create_cloud", std::span(groupLayouts, 4), sizeof(float4));


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

    renderer->BuildRenderPipeline("cloud", shaders, nullptr, std::span(&renderLayout, 1), sizeof(mat4));
}
