//
// Created by Kyle Smith on 2026-06-11.
//

#ifndef POINTMAPPER_CREATEPOINTCLOUDNODE_H
#define POINTMAPPER_CREATEPOINTCLOUDNODE_H
#include "Node.h"
#include "../../common.h"
#include "../types/CommonTypes.h"


namespace pointmapper::pipeline {
    class CreatePointCloudNode : public Node {
    public:
        CreatePointCloudNode();

        void Hydrate() override;

        void Process() override;

        std::shared_ptr<Output<GPUPointCloud>> cloud;

        std::shared_ptr<Input<GPUDepthMap>> depth_map;
        std::shared_ptr<Input<CameraParams>> camera_params;
    };
}


#endif //POINTMAPPER_CREATEPOINTCLOUDNODE_H
