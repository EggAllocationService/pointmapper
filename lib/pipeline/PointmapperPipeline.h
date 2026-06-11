//
// Created by Kyle Smith on 2026-05-22.
//
#pragma once
#include "nodes/Node.h"

#include "wgpu.h"

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

        WGPUBuffer CreateBuffer(unsigned int length, unsigned int usage);
        WGPUBindGroupLayout CreateBindGroupLayout(WGPUBindGroupLayoutEntry* entries);

        WGPUShaderModule CompileShaderModule(char* shader);
    private:
        std::vector<std::shared_ptr<Node>> nodes;
        std::vector<std::shared_ptr<Node>> roots;

        WGPUDevice device;
        WGPUQueue queue;

        bool built;
    };

}