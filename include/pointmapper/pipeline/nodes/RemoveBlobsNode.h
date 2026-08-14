//
// Created by Kyle Smith on 2026-06-16.
//

#ifndef POINTMAPPER_REMOVEBLOBSNODE_H
#define POINTMAPPER_REMOVEBLOBSNODE_H

#include "Node.h"
#include "../../common.h"
#include "../ComputePipeline.h"
#include "../types/CommonTypes.h"

namespace pointmapper::pipeline {
    class RemoveBlobsNode : public Node {
    public:
        RemoveBlobsNode();
        void Hydrate() override;

        void Process(PipelineBundle &) override;

        std::shared_ptr<Output<GPUDepthMap>> depthMap;
        std::shared_ptr<Input<GPUDepthMap>> inputDepthMap;
        std::shared_ptr<Input<CameraParams>> camera_params;
        std::shared_ptr<Input<FrameData>> frameData;

    private:
        WGPUBuffer pipelineInfoBuffer = nullptr;

        std::shared_ptr<ComputePipeline> blobPipeline;
    };
}

#endif //POINTMAPPER_REMOVEBLOBSNODE_H
