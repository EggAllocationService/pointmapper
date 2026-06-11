//
// Created by Kyle Smith on 2026-06-11.
//
#include "Node.h"

pointmapper::pipeline::PointmapperPipeline* pointmapper::pipeline::PIPELINE = nullptr;

std::vector<std::shared_ptr<pointmapper::pipeline::InputBase>>& pointmapper::pipeline::Node::GetInputs() {
    return inputs;
}

std::vector<std::shared_ptr<pointmapper::pipeline::OutputBase>>& pointmapper::pipeline::Node::GetOutputs() {
    return outputs;
}

void pointmapper::pipeline::Node::Build() {
    Hydrate();
    ready = true;
}
