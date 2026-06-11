//
// Created by Kyle Smith on 2026-06-11.
//

#include "RemoveBackgroundNode.h"

pointmapper::pipeline::RemoveBackgroundNode::RemoveBackgroundNode() {
    mask = CreateOutput<GPUMask>();
    depthMap = CreateInput<GPUDepthMap>();
}

void pointmapper::pipeline::RemoveBackgroundNode::Hydrate() {
    mask->MarkReady();
}

void pointmapper::pipeline::RemoveBackgroundNode::Process(PipelineBundle &) {
    printf("Creating depth map!\n");
}
