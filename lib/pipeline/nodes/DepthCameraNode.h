//
// Created by Kyle Smith on 2026-06-11.
//

#ifndef POINTMAPPER_DEPTHCAMERANODE_H
#define POINTMAPPER_DEPTHCAMERANODE_H

#include "Node.h"
#include "../../DepthDevice.h"
#include "../types/CommonTypes.h"

namespace pointmapper::pipeline {
    class DepthCameraNode : public Node {
    public:
        explicit DepthCameraNode(DepthDevice* device);
        void Hydrate() override;

        void Process(PipelineBundle&) override;

        std::shared_ptr<Output<GPUDepthMap>> depth;
        std::shared_ptr<Output<CameraParams>> params;
        std::shared_ptr<Output<GPUColorTexture>> color;
        std::shared_ptr<Output<GPUFrameData>> frameData;
    private:
        DepthDevice *device;
    };
}

#endif //POINTMAPPER_DEPTHCAMERANODE_H
