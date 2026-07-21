//
// Created by Kyle Smith on 2026-06-19.
//


#include "Engine.h"
#include "../lib/kinect2/Kinect2Device.h"
#include "../lib/pipeline/PointmapperPipeline.h"
#include "../lib/pipeline/nodes/DepthCameraNode.h"
#include "../lib/pipeline/nodes/NetworkReceiveNode.h"
#include "../lib/rendering/pipelines.h"
#include "../lib/rendering/PointCloudComponent.h"

class PointActor : public glengine::world::Actor {
public:
    PointActor(std::shared_ptr<pointmapper::pipeline::Output<pointmapper::pipeline::GPUPointCloud>> cloud) {
        auto component = CreateComponent<PointCloudComponent>();
        component->SetCloudNode(cloud);
    }

    void Update(double deltaTime) override {
        // animation or other per-frame calculations happen here
    }
};

int main() {
    auto engine = new glengine::Engine("Point Visualizer", int2(1280, 720));
    engine->SetAllowNonFocusedPawnInput(true);
    auto renderer = engine->GetRenderer();
    addPointmapperPipelines(renderer);

    auto pipeline = new pointmapper::pipeline::PointmapperPipeline(renderer->GetDevice(), wgpuDeviceGetQueue(renderer->GetDevice()));
    auto cam = pipeline->CreateRoot<pointmapper::pipeline::DepthCameraNode>(new Kinect2Device());

    auto cloud = pipeline->CreateNode<pointmapper::pipeline::CreatePointCloudNode>();
    cloud->camera_params->Connect(cam->params);
    cloud->color->Connect(cam->color);
    cloud->frameData->Connect(cam->frameData);
    cloud->depth_map->Connect(cam->depth);

    pipeline->Build();
    printf("Pipeline built!");

    auto actor = engine->SpawnActor<PointActor>(cloud->cloud);

    while (true) {
        glfwPollEvents();
        pipeline->Process();
        engine->Update();
        engine->Render();
    }
}
