//
// Created by Kyle Smith on 2026-05-22.
//

#include "PointmapperPipeline.h"

#include <cassert>
#include <iostream>

static void handle_request_adapter(WGPURequestAdapterStatus status,
                                   WGPUAdapter adapter, WGPUStringView message,
                                   void *userdata1, void *userdata2) {
    *(WGPUAdapter *)userdata1 = adapter;
}
static void handle_request_device(WGPURequestDeviceStatus status,
                                  WGPUDevice device, WGPUStringView message,
                                  void *userdata1, void *userdata2) {
    *(WGPUDevice *)userdata1 = device;

    std::cout << std::string_view(message.data, message.length) << std::endl;

    WGPUSupportedFeatures supportedFeatures;
    wgpuDeviceGetFeatures(device, &supportedFeatures);
    printf("Features enabled: %lu", supportedFeatures.featureCount);

    for (int i = 0; i < supportedFeatures.featureCount; i++) {
        printf("\t 0x%08x \n", supportedFeatures.features[i]);
    }
}

#define QUEUE static_cast<WGPUQueue>(this->queue)
#define DEVICE static_cast<WGPUDevice>(this->device)
#define POINT_BUFFER static_cast<WGPUBuffer>(this->pointBuffer)
#define INFO_BUFFER static_cast<WGPUBuffer>(this->infoBuffer)

PointmapperPipeline::PointmapperPipeline() {
    auto instance = wgpuCreateInstance(nullptr);
    auto adapters = new WGPUAdapter[10];
    auto adapterCount = wgpuInstanceEnumerateAdapters(instance, nullptr, adapters);

    for (int i = 0; i < adapterCount; i++) {
        printf("Adapter #%d:", i);
        WGPUAdapterInfo info;
        wgpuAdapterGetInfo(adapters[i], &info);
        std::cout << "\tName: " << std::string_view(info.device.data, info.device.length) << std::endl;
    }
    delete[] adapters;

    WGPUAdapter adapter = nullptr;
    wgpuInstanceRequestAdapter(instance, nullptr,
                           (const WGPURequestAdapterCallbackInfo){
                               .callback = handle_request_adapter,
                               .userdata1 = &adapter
                           });
    assert(adapter);

    WGPUNativeLimits nativeLimits = {
        .chain = {
            .next = nullptr,
            .sType = static_cast<WGPUSType>(WGPUSType_NativeLimits)
        },
        .maxImmediateSize = 128,
        .maxNonSamplerBindings = 0,
        .maxBindingArrayElementsPerShaderStage = 0
    };

    WGPULimits requiredLimits = WGPU_LIMITS_INIT;
    requiredLimits.nextInChain = &nativeLimits.chain;
    requiredLimits.maxImmediateSize = 128;

    auto features = new WGPUNativeFeature[2] { WGPUNativeFeature_MappablePrimaryBuffers,  WGPUNativeFeature_Immediates };
    auto deviceDescriptor = WGPU_DEVICE_DESCRIPTOR_INIT;
    deviceDescriptor.requiredLimits = &requiredLimits;
    deviceDescriptor.requiredFeatures = reinterpret_cast<WGPUFeatureName*>(&features[0]);
    deviceDescriptor.requiredFeatureCount = 2;

    WGPUDevice device = nullptr;
    wgpuAdapterRequestDevice(adapter, &deviceDescriptor,
                           (const WGPURequestDeviceCallbackInfo){
                               .callback = handle_request_device,
                               .userdata1 = &device
                           });
    assert(device);

    this->device = device;
    this->queue = wgpuDeviceGetQueue(device);

    auto infoBufferDesc = WGPU_BUFFER_DESCRIPTOR_INIT;
    infoBufferDesc.size = sizeof(uint32_t) * 4;
    infoBufferDesc.usage = WGPUBufferUsage_Storage | WGPUBufferUsage_Indirect | WGPUBufferUsage_CopyDst;
    this->infoBuffer = wgpuDeviceCreateBuffer(DEVICE, &infoBufferDesc);
    uint32_t vertexCount = 36;
    wgpuQueueWriteBuffer(QUEUE, INFO_BUFFER, 0, &vertexCount, sizeof(uint32_t));

    backgroundProcessor = std::make_shared<BackgroundProcessor>(DEVICE, QUEUE, INFO_BUFFER);
    cloudRenderer = std::make_shared<CloudRenderer>(backgroundProcessor, instance, adapter, DEVICE, INFO_BUFFER);
}

PointmapperPipeline::~PointmapperPipeline() {
    wgpuDeviceRelease(DEVICE);
    wgpuQueueRelease(QUEUE);
}

std::shared_ptr<BackgroundProcessor> PointmapperPipeline::GetBackgroundProcessor() {
    return backgroundProcessor;
}

std::shared_ptr<CloudRenderer> PointmapperPipeline::GetCloudRenderer() {
    return cloudRenderer;
}

void PointmapperPipeline::resize(CameraParams params) {
    if (pointBuffer != nullptr) {
        wgpuBufferRelease(POINT_BUFFER);
    }

    auto infoBufferDesc = WGPU_BUFFER_DESCRIPTOR_INIT;
    infoBufferDesc.size = params.width * params.height * sizeof(PointXYZRGB);
    infoBufferDesc.usage = WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc;
    this->pointBuffer = wgpuDeviceCreateBuffer(DEVICE, &infoBufferDesc);

    backgroundProcessor->resize(params, POINT_BUFFER);
    cloudRenderer->resize(POINT_BUFFER);
}
