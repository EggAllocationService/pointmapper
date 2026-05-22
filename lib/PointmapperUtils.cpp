//
// Created by Kyle Smith on 2026-05-22.
//

#include "PointmapperUtils.h"

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

PointmapperUtils::PointmapperUtils() {
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
}

PointmapperUtils::~PointmapperUtils() {
    wgpuDeviceRelease(DEVICE);
    wgpuQueueRelease(QUEUE);
}

std::unique_ptr<BackgroundProcessor> PointmapperUtils::GetBackgroundProcessor() {
    return std::make_unique<BackgroundProcessor>(DEVICE, QUEUE);
}
