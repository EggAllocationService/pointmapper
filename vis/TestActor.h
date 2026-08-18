//
// Created by Kyle Smith on 2026-06-02.
//
#pragma once
#include "pointmapper/rendering/PointCloudComponent.h"
#include "3d/Actor.h"
#include "3d/text/SlugTextComponent.h"


class TestActor : public glengine::world::Actor {
public:
    TestActor();
    void SetNode(std::shared_ptr<pointmapper::pipeline::Output<pointmapper::pipeline::GPUPointCloud>> node);
    void Update(double deltaTime) override;
    void SetTint(float4 tint) {
        pc_->SetTint(tint);
    }
    void SetTintDepth(float depth) {
        pc_->SetMaxDepth(depth);
    }
private:
    std::shared_ptr<glengine::world::font::SlugTextComponent> text_;
    std::shared_ptr<PointCloudComponent> pc_;
};
