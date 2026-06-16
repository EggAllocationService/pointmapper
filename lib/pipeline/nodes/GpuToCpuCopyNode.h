//
// Created by Kyle Smith on 2026-06-16.
//

#ifndef POINTMAPPER_GPUTOCPUCOPYNODE_H
#define POINTMAPPER_GPUTOCPUCOPYNODE_H

#include "Node.h"
#include "../../common.h"
#include "../types/CommonTypes.h"

#include <webgpu/webgpu.h>

namespace pointmapper::pipeline {
    class GpuToCpuCopyNode : public Node {
    public:
        GpuToCpuCopyNode();
        ~GpuToCpuCopyNode() override;

        void Hydrate() override;
        void Process(PipelineBundle&) override;

        std::shared_ptr<Input<GPUPointCloud>> cloud;
        std::shared_ptr<Output<CPUPointCloud>> cpuCloud;

    private:
        WGPUBuffer stagingPoints = nullptr;
        WGPUBuffer stagingPointCount = nullptr;
        size_t pointCapacityBytes = 0;

        void ReadbackBuffer(WGPUBuffer buffer, size_t size);
    };
}

#endif //POINTMAPPER_GPUTOCPUCOPYNODE_H
