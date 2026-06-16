//
// Created by Kyle Smith on 2026-05-22.
//

#include "PointmapperPipeline.h"
#include "../common.h"

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
    built = false;
}

pointmapper::pipeline::PointmapperPipeline::PointmapperPipeline(WGPUDevice device, WGPUQueue queue) {
    this->device = device;
    this->queue = queue;
    built = false;
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
    auto encoderDesc = WGPUCommandEncoderDescriptor {
        .nextInChain = nullptr,
        .label = {}
    };
    auto encoder = wgpuDeviceCreateCommandEncoder(device, &encoderDesc);

    auto passDesc = WGPUComputePassDescriptor {
        .nextInChain = nullptr,
        .label = {},
        .timestampWrites = nullptr
    };
    auto pass = wgpuCommandEncoderBeginComputePass(encoder, &passDesc);

    auto bundle = PipelineBundle {
        .cmd = encoder,
        .encoder = pass,
        .queue = queue,
        .device = device
    };

    for (auto x : executionOrder) {
        x->LazyProcess(bundle);
    }

    wgpuComputePassEncoderEnd(pass);

    auto cmd = wgpuCommandEncoderFinish(encoder, nullptr);
    wgpuQueueSubmit(queue, 1, &cmd);

    wgpuCommandBufferRelease(cmd);
    wgpuComputePassEncoderRelease(pass);
    wgpuCommandEncoderRelease(encoder);
}

WGPUBuffer pointmapper::pipeline::PointmapperPipeline::CreateBuffer(unsigned int length, WGPUBufferUsage usage) const {
    auto desc = WGPUBufferDescriptor {
        .nextInChain = nullptr,
        .label = {},
        .usage = usage,
        .size = length,
        .mappedAtCreation = false
    };

    return wgpuDeviceCreateBuffer(device, &desc);
}

WGPUBindGroupLayout pointmapper::pipeline::PointmapperPipeline::
CreateBindGroupLayout(const WGPUBindGroupLayoutEntry *entries, const unsigned long count) const {
    const auto desc = WGPUBindGroupLayoutDescriptor {
        .nextInChain = nullptr,
        .label = {},
        .entryCount = count,
        .entries = entries
    };
    return wgpuDeviceCreateBindGroupLayout(device, &desc);
}

WGPUShaderModule pointmapper::pipeline::PointmapperPipeline::CompileShaderModule(const char *shader) const {

    WGPUShaderSourceWGSL source = WGPU_SHADER_SOURCE_WGSL_INIT;
    source.code = {
        .data = shader,
        .length = WGPU_STRLEN
    };

    auto desc = WGPUShaderModuleDescriptor {
        .nextInChain = &source.chain,
        .label = {}
    };
    return wgpuDeviceCreateShaderModule(device, &desc);
}

std::shared_ptr<pointmapper::pipeline::ComputePipeline> pointmapper::pipeline::PointmapperPipeline::
GetComputePipelineByName(const std::string& name) const {
    auto it = computePipelines.find(name);
    if (it != computePipelines.end()) {
        return it->second;
    }
    return nullptr;
}

std::shared_ptr<pointmapper::pipeline::ComputePipeline> pointmapper::pipeline::PointmapperPipeline::
BuildComputePipeline(std::string name, WGPUShaderModule kernel, std::string_view entryPoint,
                    std::span<WGPUBindGroupLayoutDescriptor> bindGroups, uint32_t immediateDataBytes) {
    std::vector<WGPUBindGroupLayout> layouts;
    layouts.reserve(bindGroups.size());
    for (auto& desc : bindGroups) {
        layouts.push_back(wgpuDeviceCreateBindGroupLayout(device, &desc));
    }

    WGPUPipelineLayoutExtras extras = {
        .chain = {
            .next = nullptr,
            .sType = static_cast<WGPUSType>(WGPUSType_PipelineLayoutExtras)
        },
        .immediateDataSize = immediateDataBytes
    };

    auto layoutDesc = WGPUPipelineLayoutDescriptor {
        .nextInChain = &extras.chain,
        .label = {},
        .bindGroupLayoutCount = layouts.size(),
        .bindGroupLayouts = layouts.data(),
        .immediateSize = immediateDataBytes
    };

    auto desc = WGPUComputePipelineDescriptor {
        .nextInChain = nullptr,
        .label = {
            .data = name.data(),
            .length = name.length()
        },
        .layout = wgpuDeviceCreatePipelineLayout(device, &layoutDesc),
        .compute = {
            .nextInChain = nullptr,
            .module = kernel,
            .entryPoint = {
                .data = entryPoint.data(),
                .length = entryPoint.length()
            },
            .constantCount = 0,
            .constants = nullptr
        }
    };

    auto pipeline = wgpuDeviceCreateComputePipeline(device, &desc);

    auto built = std::make_shared<ComputePipeline>(device, pipeline, std::move(layouts), immediateDataBytes);
    computePipelines.insert_or_assign(name, built);
    return built;
}

std::shared_ptr<pointmapper::pipeline::GPUTexture> pointmapper::pipeline::PointmapperPipeline::
CreateTexture(std::string_view name, WGPUTextureUsage usage, WGPUTextureFormat format,
              unsigned int width, unsigned int height) const {
    auto desc = WGPUTextureDescriptor {
        .nextInChain = nullptr,
        .label = {
            .data = name.data(),
            .length = name.length()
        },
        .usage = usage,
        .dimension = WGPUTextureDimension_2D,
        .size = {
            .width = width,
            .height = height,
            .depthOrArrayLayers = 1
        },
        .format = format,
        .mipLevelCount = 1,
        .sampleCount = 1,
        .viewFormatCount = 0,
        .viewFormats = nullptr
    };

    return std::make_shared<GPUTexture>(wgpuDeviceCreateTexture(device, &desc), format, width, height);
}
