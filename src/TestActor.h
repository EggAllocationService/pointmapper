//
// Created by Kyle Smith on 2026-06-02.
//
#pragma once
#include "../lib/rendering/PointCloudComponent.h"
#include "3d/Actor.h"
#include "3d/text/SlugTextComponent.h"


class TestActor : public glengine::world::Actor {
public:
    TestActor();
    void SetDevice(DepthDevice *dev);
    void Update(double deltaTime) override;
    void SetRegistration(mat4 r) {
        pc_->SetRegistration(r);
    }
private:
    std::shared_ptr<glengine::world::font::SlugTextComponent> text_;
    std::shared_ptr<PointCloudComponent> pc_;
};
