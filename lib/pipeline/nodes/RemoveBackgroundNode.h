//
// Created by Kyle Smith on 2026-06-11.
//

#ifndef POINTMAPPER_REMOVEBACKGROUNDNODE_H
#define POINTMAPPER_REMOVEBACKGROUNDNODE_H

#include "Node.h"
#include "../../common.h"
#include "../ComputePipeline.h"
#include "../types/CommonTypes.h"

namespace pointmapper::pipeline {
    class RemoveBackgroundNode : public Node {
    public:
        RemoveBackgroundNode();
        void Hydrate() override;

        void Process(PipelineBundle &) override;

        std::shared_ptr<Output<GPUDepthMap>> depthMap;
        std::shared_ptr<Input<GPUDepthMap>> inputDepthMap;
        std::shared_ptr<Input<CameraParams>> camera_params;

    private:
        WGPUBuffer pipelineInfoBuffer = nullptr;
        WGPUBuffer dummyOutInfo = nullptr;
        WGPUBuffer maxDepth = nullptr;
        WGPUBuffer prevDepth = nullptr;

        std::shared_ptr<ComputePipeline> maskPipeline;
    };
}

#endif //POINTMAPPER_REMOVEBACKGROUNDNODE_H
