//
// Created by Kyle Smith on 2026-05-13.
//

#include <cstdio>
#include <webgpu/webgpu.h>
#include <iostream>
const char kernels[] = {
#embed "kernels.wgsl"
};

#include "BackgroundProcessor.h"
#include "../common.h"
#include <assert.h>

struct MaskPushConstants {
    float scale;
};

static void handle_buffer_map(WGPUMapAsyncStatus status,
                              WGPUStringView message,
                              void *userdata1, void *userdata2) {
    //
}

BackgroundProcessor::BackgroundProcessor(WGPUDevice device, WGPUQueue queue, WGPUBuffer infoBuf) {
    this->device = device;
    this->queue = queue;
    this->outputInfo = infoBuf;

    auto kernelSource = WGPU_SHADER_SOURCE_WGSL_INIT;
    kernelSource.code.data = kernels;
    kernelSource.code.length = sizeof(kernels);

    auto kernelDescriptor = WGPU_SHADER_MODULE_DESCRIPTOR_INIT;
    kernelDescriptor.nextInChain = &kernelSource.chain;
    this->kernelModule = wgpuDeviceCreateShaderModule(device, &kernelDescriptor);

    createPipelines();
}

void BackgroundProcessor::processFrame(const std::shared_ptr<Frame>& frame, std::vector<PointXYZRGB>& pointCloud) {
    pointCloud.resize(info.width * info.height);

    // begin upload immediately
    wgpuQueueWriteBuffer(queue, depth, 0, frame->GetDepth(), info.width * info.height * sizeof(float));

    // reset result info buffer
    uint32_t newValue = 0;
    wgpuQueueWriteBuffer(queue, outputInfo, 4, &newValue, sizeof(uint32_t));

    auto texelCopyInfo = WGPU_TEXEL_COPY_TEXTURE_INFO_INIT;
    texelCopyInfo.aspect = WGPUTextureAspect_All;
    texelCopyInfo.texture = colorTexture;

    auto bufLayout = WGPU_TEXEL_COPY_BUFFER_LAYOUT_INIT;
    bufLayout.bytesPerRow = info.colorWidth * 4;
    bufLayout.rowsPerImage = info.colorHeight;

    auto extent = WGPU_EXTENT_3D_INIT;
    extent.width = info.colorWidth;
    extent.height = info.colorHeight;
    extent.depthOrArrayLayers = 1;
    wgpuQueueWriteTexture(queue, &texelCopyInfo, frame->GetColor(), 4 * info.colorWidth * info.colorHeight, &bufLayout, &extent);
    wgpuQueueSubmit(queue, 0, nullptr);

    auto encoder = wgpuDeviceCreateCommandEncoder(device, nullptr);

    // depth masking pass
    auto maskPass = wgpuCommandEncoderBeginComputePass(encoder, nullptr);
    wgpuComputePassEncoderSetBindGroup(maskPass, 0, bindGroups[0], 0, nullptr);
    wgpuComputePassEncoderSetBindGroup(maskPass, 1, bindGroups[1], 0, nullptr);
    wgpuComputePassEncoderSetBindGroup(maskPass, 2, bindGroups[2], 0, nullptr);
    wgpuComputePassEncoderSetPipeline(maskPass, maskPipeline);
    MaskPushConstants constants = {
        .scale = frame->GetDepthUnits()
    };
    wgpuComputePassEncoderSetImmediates(maskPass, 0, sizeof(MaskPushConstants), &constants);
    wgpuComputePassEncoderDispatchWorkgroups(maskPass, info.width, info.height, 1);
    wgpuComputePassEncoderEnd(maskPass);

    auto cloudPass = wgpuCommandEncoderBeginComputePass(encoder, nullptr);
    wgpuComputePassEncoderSetBindGroup(cloudPass, 0, bindGroups[0], 0, nullptr);
    wgpuComputePassEncoderSetBindGroup(cloudPass, 1, bindGroups[1], 0, nullptr);
    wgpuComputePassEncoderSetBindGroup(cloudPass, 2, bindGroups[2], 0, nullptr);
    wgpuComputePassEncoderSetBindGroup(cloudPass, 3, bindGroups[3], 0, nullptr);
    wgpuComputePassEncoderSetPipeline(cloudPass, cloudPipeline);
    wgpuComputePassEncoderDispatchWorkgroups(cloudPass, info.width, info.height, 1);
    wgpuComputePassEncoderEnd(cloudPass);

    auto outputSize = info.width * info.height * sizeof(PointXYZRGB);
   wgpuCommandEncoderCopyBufferToBuffer(encoder, output, 0,
        outputCopy, 0,
        outputSize);

    auto commandBuffer = wgpuCommandEncoderFinish(encoder, nullptr);
    wgpuCommandEncoderRelease(encoder);

    wgpuQueueSubmit(queue, 1, &commandBuffer);
    wgpuCommandBufferRelease(commandBuffer);

    wgpuBufferMapAsync(outputCopy, WGPUMapMode_Read, 0, outputSize,
        (const WGPUBufferMapCallbackInfo){
                               .callback = handle_buffer_map
                           });
    wgpuDevicePoll(device, true, nullptr);

    auto results = wgpuBufferGetMappedRange(outputCopy, 0, outputSize);
    memcpy(pointCloud.data(), results, outputSize);

    wgpuBufferUnmap(outputCopy);
}


void BackgroundProcessor::resize(int depthWidth, int depthHeight, int colorW, int colorH, float fx, float fy, float cx, float cy, ColorType colorType) {
    info.cx = cx;
    info.cy = cy;
    info.fx = fx;
    info.fy = fy;
    info.height = depthHeight;
    info.width = depthWidth;
    info.colorWidth = colorW;
    info.colorHeight = colorH;
    info.depth_tolerance = 0.5f;

    if (this->depth != nullptr) {
        wgpuBufferRelease(depth);
        wgpuBufferRelease(maxDepth);
        wgpuBufferRelease(prevDepth);
        wgpuBufferRelease(uniforms);
        wgpuBufferRelease(outputCopy);
        wgpuTextureRelease(colorTexture);
        wgpuSamplerRelease(sampler);

        for (int i = 0; i < 3; i++) {
            wgpuBindGroupRelease(bindGroups[i]);
        }
    }

    auto depthBufferDescriptor = WGPU_BUFFER_DESCRIPTOR_INIT;
    depthBufferDescriptor.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc;
    depthBufferDescriptor.size = sizeof(uint32_t) * depthHeight * depthWidth;
    this->depth = wgpuDeviceCreateBuffer(device, &depthBufferDescriptor);
    depthBufferDescriptor.usage = WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst;
    this->maxDepth = wgpuDeviceCreateBuffer(device, &depthBufferDescriptor);
    this->prevDepth = wgpuDeviceCreateBuffer(device, &depthBufferDescriptor);

    auto uniformBufferDescriptor = WGPU_BUFFER_DESCRIPTOR_INIT;
    uniformBufferDescriptor.usage = WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst;
    uniformBufferDescriptor.size = sizeof(PipelineInfo);
    this->uniforms = wgpuDeviceCreateBuffer(device, &uniformBufferDescriptor);
    wgpuQueueWriteBuffer(queue, uniforms, 0, &info, sizeof(PipelineInfo)); // begin upload immediately

    auto outputBufferDescriptor = WGPU_BUFFER_DESCRIPTOR_INIT;
    outputBufferDescriptor.size = sizeof(PointXYZRGB) * depthHeight * depthWidth;
    outputBufferDescriptor.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead;
    this->outputCopy = wgpuDeviceCreateBuffer(device, &outputBufferDescriptor);

    auto colorTexDescriptor = WGPU_TEXTURE_DESCRIPTOR_INIT;
    colorTexDescriptor.dimension = WGPUTextureDimension_2D;
    colorTexDescriptor.format = colorType == RGBX ? WGPUTextureFormat_RGBA8Unorm : WGPUTextureFormat_BGRA8Unorm;
    colorTexDescriptor.usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst;
    colorTexDescriptor.size.width = colorW;
    colorTexDescriptor.size.height = colorH;
    colorTexDescriptor.size.depthOrArrayLayers = 1;

    this->colorTexture = wgpuDeviceCreateTexture(device, &colorTexDescriptor);

    this->colorTextureView = wgpuTextureCreateView(colorTexture, nullptr);

    this->sampler = wgpuDeviceCreateSampler(device, nullptr);

    auto group0Entries = new WGPUBindGroupEntry[2] {WGPU_BIND_GROUP_ENTRY_INIT, WGPU_BIND_GROUP_ENTRY_INIT};
    group0Entries[0].buffer = uniforms;
    group0Entries[1].buffer = outputInfo;
    group0Entries[1].binding = 1;
    auto group0Desc = WGPU_BIND_GROUP_DESCRIPTOR_INIT;
    group0Desc.layout = bindLayouts[0];
    group0Desc.entryCount = 2;
    group0Desc.entries = &group0Entries[0];
    bindGroups[0] = wgpuDeviceCreateBindGroup(device, &group0Desc);

    auto group1Entries = new WGPUBindGroupEntry[2] {WGPU_BIND_GROUP_ENTRY_INIT, WGPU_BIND_GROUP_ENTRY_INIT};
    group1Entries[0].buffer = depth;
    group1Entries[1].buffer = output;
    group1Entries[1].binding = 1;
    auto group1Desc = WGPU_BIND_GROUP_DESCRIPTOR_INIT;
    group1Desc.layout = bindLayouts[1];
    group1Desc.entryCount = 2;
    group1Desc.entries = &group1Entries[0];
    bindGroups[1] = wgpuDeviceCreateBindGroup(device, &group1Desc);
    delete[] group1Entries;

    auto group2Entries = new WGPUBindGroupEntry[2] {WGPU_BIND_GROUP_ENTRY_INIT, WGPU_BIND_GROUP_ENTRY_INIT};
    group2Entries[0].buffer = maxDepth;
    group2Entries[1].buffer = prevDepth;
    group2Entries[1].binding = 1;
    auto group2Desc = WGPU_BIND_GROUP_DESCRIPTOR_INIT;
    group2Desc.layout = bindLayouts[2];
    group2Desc.entryCount = 2;
    group2Desc.entries = &group2Entries[0];
    bindGroups[2] = wgpuDeviceCreateBindGroup(device, &group2Desc);
    delete[] group2Entries;


    auto group3Entries = new WGPUBindGroupEntry[2] {WGPU_BIND_GROUP_ENTRY_INIT, WGPU_BIND_GROUP_ENTRY_INIT};
    group3Entries[0].sampler = sampler;
    group3Entries[1].textureView = colorTextureView;
    group3Entries[1].binding = 1;
    auto group3Desc = WGPU_BIND_GROUP_DESCRIPTOR_INIT;
    group3Desc.layout = bindLayouts[3];
    group3Desc.entryCount = 2;
    group3Desc.entries = &group3Entries[0];
    bindGroups[3] = wgpuDeviceCreateBindGroup(device, &group3Desc);
    delete[] group3Entries;

    wgpuDevicePoll(device, true, nullptr);
    printf("Buffers setup");
}

void BackgroundProcessor::resize(CameraParams params, WGPUBuffer pointBuffer) {
    this->output = pointBuffer;
    resize(params.width, params.height, params.colorWidth, params.colorHeight, params.fx, params.fy, params.cx, params.cy, params.colorType);
}

void BackgroundProcessor::recalibrate() {
    auto encoder = wgpuDeviceCreateCommandEncoder(device, nullptr);
    wgpuCommandEncoderCopyBufferToBuffer(encoder, depth, 0, maxDepth, 0, sizeof(float) * info.width * info.height);
    auto cmd = wgpuCommandEncoderFinish(encoder, nullptr);

    wgpuQueueSubmit(queue, 1, &cmd);
    wgpuCommandBufferRelease(cmd);
}

PointmapperDeviceCloud BackgroundProcessor::getOuputBuffer() const {
    return output;
}

void BackgroundProcessor::createPipelines() {
//{ Layout Creation
    // create pipeline layouts manually to support sharing

    // group 0 - uniforms
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


    bindLayouts[0] = wgpuDeviceCreateBindGroupLayout(device, &groupLayouts[0]);
    bindLayouts[1] = wgpuDeviceCreateBindGroupLayout(device, &groupLayouts[1]);
    bindLayouts[2] = wgpuDeviceCreateBindGroupLayout(device, &groupLayouts[2]);
    bindLayouts[3] = wgpuDeviceCreateBindGroupLayout(device, &groupLayouts[3]);

    auto pipelineLayoutDesc = WGPU_PIPELINE_LAYOUT_DESCRIPTOR_INIT;
    pipelineLayoutDesc.bindGroupLayouts = &bindLayouts[0];
    pipelineLayoutDesc.bindGroupLayoutCount = 3;
    WGPUPipelineLayoutExtras extras = {
        .chain = {
            .next = nullptr,
            .sType = static_cast<WGPUSType>(WGPUSType_PipelineLayoutExtras),
        },
        .immediateDataSize = sizeof(MaskPushConstants)
    };

    pipelineLayoutDesc.nextInChain = &extras.chain;
    pipelineLayoutDesc.immediateSize = sizeof(MaskPushConstants);
    auto maskPipelineLayout = wgpuDeviceCreatePipelineLayout(device, &pipelineLayoutDesc);

    pipelineLayoutDesc.immediateSize = 0;
    pipelineLayoutDesc.bindGroupLayoutCount = 4;
    auto cloudPipelineLayout = wgpuDeviceCreatePipelineLayout(device, &pipelineLayoutDesc);

    delete[] groupLayouts;
    delete[] group1Bindings;
//}

    auto maskPipelineDesc = WGPU_COMPUTE_PIPELINE_DESCRIPTOR_INIT;
    maskPipelineDesc.layout = maskPipelineLayout;
    maskPipelineDesc.compute.module = kernelModule;
    maskPipelineDesc.compute.entryPoint = {
        .data = "mask",
        .length = 4,
    };

    this->maskPipeline = wgpuDeviceCreateComputePipeline(device, &maskPipelineDesc);

    auto cloudPipelineDesc = WGPU_COMPUTE_PIPELINE_DESCRIPTOR_INIT;
    cloudPipelineDesc.layout = cloudPipelineLayout;
    cloudPipelineDesc.compute.module = kernelModule;
    cloudPipelineDesc.compute.entryPoint = {
        .data = "create_cloud",
        .length = 12,
    };
    this->cloudPipeline = wgpuDeviceCreateComputePipeline(device, &cloudPipelineDesc);
}
