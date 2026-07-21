//
// Created by Kyle Smith on 2026-06-19.
//

#ifndef POINTMAPPER_CPUTOGPUCOPYNODE_H
#define POINTMAPPER_CPUTOGPUCOPYNODE_H
#include "Node.h"
#include "../types/CommonTypes.h"

namespace pointmapper::pipeline {
    class CpuToGpuCopyNode : public Node {
    public:
        CpuToGpuCopyNode();
        void Hydrate() override;

        void Process(PipelineBundle &) override;

        std::shared_ptr<Output<GPUPointCloud>> output;
        std::shared_ptr<Input<CPUPointCloud>> input;
    };
}


#endif //POINTMAPPER_CPUTOGPUCOPYNODE_H
