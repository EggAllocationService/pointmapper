//
// Created by Kyle Smith on 2026-05-22.
//

#include "PointmapperPipeline.h"
#include "../common.h"
#include "wgpu.h"

#include <cassert>
#include <iostream>
#include <list>

static void handle_request_adapter(WGPURequestAdapterStatus status,
                                   WGPUAdapter adapter, WGPUStringView message,
                                   void *userdata1, void *userdata2) {
    *(WGPUAdapter *)userdata1 = adapter;

    WGPUAdapterInfo info;
    wgpuAdapterGetInfo(adapter, &info);
    std::cout << std::string_view(info.device.data, info.device.length) << std::endl;
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

pointmapper::pipeline::PointmapperPipeline::PointmapperPipeline() {
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
    WGPURequestAdapterOptions options = {
        .nextInChain = nullptr,
        .featureLevel = WGPUFeatureLevel_Core,
        .powerPreference = WGPUPowerPreference_HighPerformance,
        .forceFallbackAdapter = false,
        .backendType = WGPUBackendType_Undefined,
        .compatibleSurface = nullptr
    };
    wgpuInstanceRequestAdapter(instance, &options,
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

pointmapper::pipeline::PointmapperPipeline::PointmapperPipeline(WGPUDevice device, WGPUQueue queue) {
    this->device = device;
    this->queue = queue;
}

pointmapper::pipeline::PointmapperPipeline::~PointmapperPipeline() {
    wgpuDeviceRelease(device);
    wgpuQueueRelease(queue);
}

void pointmapper::pipeline::PointmapperPipeline::Build() {
    PIPELINE = this;

    std::list<std::shared_ptr<Node>> toVisit;

    toVisit.insert(toVisit.end(), roots.begin(), roots.end());

    while (!toVisit.empty()) {
        auto node = toVisit.front();
        toVisit.pop_front();

        if (node->WasBuilt()) {
            continue;
        }

        // check that all inputs are connected & ready
        bool ok = true;
        for (const auto& input : node->GetInputs()) {
            if (!input->IsConnected()) {
                ok = false;
                break;
            }
        }
        if (!ok) {
            // an input is not ready, probably waiting for another root to prep its part of the graph.
            // push it to the back
            toVisit.push_back(node);
        } else {
            node->Build();

            executionOrder.push_back(node.get());
            // push all dependant nodes
            for (const auto& output : node->GetOutputs()) {
                auto targets = output->GetTargets();
                for (const auto& target : targets) {
                    toVisit.push_back(target->GetNode());
                }
            }
        }
    }

    built = true;
}

void pointmapper::pipeline::PointmapperPipeline::Process() {
    auto bundle = PipelineBundle();
    for (auto x : executionOrder) {
        x->Process(bundle);
    }
}

WGPUBuffer pointmapper::pipeline::PointmapperPipeline::CreateBuffer(unsigned int length, unsigned int usage) {
}
