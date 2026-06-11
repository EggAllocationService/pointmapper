//
// Created by Kyle Smith on 2026-06-11.
//

#include "DepthCameraNode.h"

pointmapper::pipeline::DepthCameraNode::DepthCameraNode(DepthDevice* d) {
    depth = CreateOutput<GPUDepthMap>();
    params = CreateOutput<CameraParams>();
    device = d;
}

void pointmapper::pipeline::DepthCameraNode::Hydrate() {
    auto& x = **params;
    x = device->GetCameraParameters();

    auto& out = **depth;
    out.height = x.height;
    out.width = x.width;

    params->MarkReady();
    depth->MarkReady();

}

void pointmapper::pipeline::DepthCameraNode::Process(PipelineBundle& bundle) {
    printf("Reading Camera\n");
}
