//
// Created by Kyle Smith on 2026-06-02.
//

#include "TestActor.h"

#include "Engine.h"
#include "3d/text/SlugTextComponent.h"
#include "../lib/kinect2/Kinect2Device.h"
#include "../lib/rendering/PointCloudComponent.h"

TestActor::TestActor() {
    pc_ = CreateComponent<PointCloudComponent>();
    pc_->GetTransform()->SetScale({4, 4, 4});

    text_ = CreateComponent<glengine::world::font::SlugTextComponent>();
    text_->SetFont(
        GetEngine()->GetResourceManager()->GetResource<glengine::world::font::Font>("/builtin/trim.ttf")
    );
    text_->GetTransform()->SetScale({0.5, 0.5, 0.5});
    text_->SetText("Unknown");
}

void TestActor::SetDevice(DepthDevice *dev) {
    auto name = dev->GetName();

    pc_->SetDevice(dev);
    text_->SetText(name);
}

void TestActor::Update(double deltaTime) {
}
