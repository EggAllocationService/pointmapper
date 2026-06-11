//
// Created by Kyle Smith on 2026-06-11.
//

#ifndef POINTMAPPER_REMOVEBACKGROUNDNODE_H
#define POINTMAPPER_REMOVEBACKGROUNDNODE_H
#include "Node.h"
#include "../types/CommonTypes.h"


namespace pointmapper::pipeline {
    class RemoveBackgroundNode : public Node {
    public:
        RemoveBackgroundNode();
        void Hydrate() override;

        void Process(PipelineBundle &) override;

        std::shared_ptr<Output<GPUMask>> mask;
        std::shared_ptr<Input<GPUDepthMap>> depthMap;
    };
} // pipeline
// pointmapper

#endif //POINTMAPPER_REMOVEBACKGROUNDNODE_H
