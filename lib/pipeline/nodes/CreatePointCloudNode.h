//
// Created by Kyle Smith on 2026-06-11.
//

#ifndef POINTMAPPER_CREATEPOINTCLOUDNODE_H
#define POINTMAPPER_CREATEPOINTCLOUDNODE_H

#include "Node.h"
#include "../../common.h"
#include "../ComputePipeline.h"
#include "../types/CommonTypes.h"

namespace pointmapper::pipeline {
    class CreatePointCloudNode : public Node {
    public:
        CreatePointCloudNode();

        void Hydrate() override;

        void Process(PipelineBundle&) override;

        std::shared_ptr<Output<GPUPointCloud>> cloud;

        std::shared_ptr<Input<GPUDepthMap>> depth_map;
        std::shared_ptr<Input<CameraParams>> camera_params;
        std::shared_ptr<Input<GPUColorTexture>> color;
        std::shared_ptr<Input<GPUFrameData>> frameData;

    private:
        WGPUBuffer pipelineInfoBuffer = nullptr;
        WGPUBuffer pointCountBuffer = nullptr;
        WGPUSampler sampler = nullptr;

        std::shared_ptr<ComputePipeline> cloudPipeline;
    };
}

#endif //POINTMAPPER_CREATEPOINTCLOUDNODE_H
