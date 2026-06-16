//
// Created by Kyle Smith on 2026-05-22.
//
#pragma once
#include "ComputePipeline.h"
#include "GPUTexture.h"
#include "nodes/Node.h"

#include "webgpu/wgpu.h"

#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>

struct CameraParams;

/// Allows related objects to share GPU-side resources
namespace pointmapper::pipeline {

    class PointmapperPipeline {
    public:
        PointmapperPipeline();

        PointmapperPipeline(WGPUDevice device, WGPUQueue queue);

        ~PointmapperPipeline();

        template<typename T, typename... Args>  std::shared_ptr<T> CreateNode(Args&&... args) {
            if (built) {
                printf("Tried adding node after building!");
                std::abort();
            }
            PIPELINE = this;
            auto node = std::make_shared<T>(std::forward<Args>(args)...);
            nodes.push_back(node);
            return node;
        }

        template<typename T, typename... Args>  std::shared_ptr<T> CreateRoot(Args&&... args) {
            if (built) {
                printf("Tried adding node after building!");
                std::abort();
            }
            PIPELINE = this;
            auto node = std::make_shared<T>(std::forward<Args>(args)...);
            roots.push_back(node);
            return node;
        }

        void Build();

        void Process();

        [[nodiscard]] WGPUBuffer CreateBuffer(unsigned int length, WGPUBufferUsage usage) const;
        [[nodiscard]] WGPUBindGroupLayout CreateBindGroupLayout(const WGPUBindGroupLayoutEntry* entries, unsigned long count) const;

        [[nodiscard]] WGPUShaderModule CompileShaderModule(const char* shader) const;

        [[nodiscard]] WGPUDevice GetDevice() const { return device; }
        [[nodiscard]] WGPUQueue GetQueue() const { return queue; }

        [[nodiscard]] std::shared_ptr<ComputePipeline> GetComputePipelineByName(const std::string& name) const;
        [[nodiscard]] std::shared_ptr<ComputePipeline> BuildComputePipeline(std::string name, WGPUShaderModule kernel,
            std::string_view entryPoint, std::span<WGPUBindGroupLayoutDescriptor> bindGroups,
            uint32_t immediateDataBytes);

        [[nodiscard]] std::shared_ptr<GPUTexture> CreateTexture(std::string_view name, WGPUTextureUsage usage,
            WGPUTextureFormat format, unsigned int width, unsigned int height) const;

    private:
        std::vector<std::shared_ptr<Node>> nodes;
        std::vector<std::shared_ptr<Node>> roots;
        std::vector<Node*> executionOrder;

        std::unordered_map<std::string, std::shared_ptr<ComputePipeline>> computePipelines;

        WGPUDevice device;
        WGPUQueue queue;

        bool built;
    };

}