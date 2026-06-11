//
// Created by Kyle Smith on 2026-06-11.
//

#include "CreatePointCloudNode.h"


namespace pointmapper::pipeline {
    CreatePointCloudNode::CreatePointCloudNode() {
        cloud = CreateOutput<GPUPointCloud>();

        depth_map = CreateInput<GPUDepthMap>();
        camera_params = CreateInput<CameraParams>();
        mask = CreateInput<GPUMask>();
    }

    void CreatePointCloudNode::Hydrate() {
    }

    void CreatePointCloudNode::Process(PipelineBundle&) {
        printf("Creating Cloud\n");
    }
}