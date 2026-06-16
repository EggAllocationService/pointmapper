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

void pointmapper::pipeline::Node::LazyProcess(PipelineBundle &bundle) {

    for (const auto& input : inputs) {
        if (input->HasNewData()) {
            Process(bundle);

            // notify all downstream nodes that there's a new output waiting
            for (auto& output : outputs) {
                for (auto target : output->GetTargets()) {
                    target->Notify();
                }
            }

            return;
        }
    }
}
