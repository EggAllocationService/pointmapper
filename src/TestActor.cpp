//
// Created by Kyle Smith on 2026-06-02.
//

#include "TestActor.h"

#include "Engine.h"
#include "3d/text/SlugTextComponent.h"
#include "../lib/kinect2/Kinect2Device.h"
#include "../lib/rendering/PointCloudComponent.h"
#include "3d/mesh/StaticMeshComponent.h"

TestActor::TestActor() {
    pc_ = CreateComponent<PointCloudComponent>();
    pc_->GetTransform()->SetScale({4, 4, 4});

    text_ = CreateComponent<glengine::world::font::SlugTextComponent>();
    text_->SetFont(
        GetEngine()->GetResourceManager()->GetResource<glengine::world::font::Font>("/builtin/fonts/trim.ttf")
    );
    text_->GetTransform()->SetScale({0.5, 0.5, 0.5});
    text_->SetText("Unknown");

    auto m = CreateComponent<glengine::world::mesh::StaticMeshComponent>();
    m->SetMesh(
        GetEngine()->GetResourceManager()->GetResource<glengine::world::mesh::StaticMesh>("/builtin/models/cube.obj")
    );
    m->GetTransform()->SetPosition({0, -2, 0});
    m->material->Ambient = float4(0.3, 0.3, 0.3, 1.0);
}

void TestActor::SetNode(std::shared_ptr<pointmapper::pipeline::CreatePointCloudNode>  node) {
    text_->SetText("Connected");

    pc_->SetCloudNode(node);
}

void TestActor::Update(double deltaTime) {
}
