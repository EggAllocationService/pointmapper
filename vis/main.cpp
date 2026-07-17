//
// Created by Kyle Smith on 2026-06-19.
//


#include "Engine.h"
#include "TestActor.h"
#include "../lib/kinect2/Kinect2Device.h"
#include "../lib/pipeline/PointmapperPipeline.h"
#include "../lib/pipeline/nodes/CpuToGpuCopyNode.h"
#include "../lib/pipeline/nodes/DepthCameraNode.h"
#include "../lib/pipeline/nodes/DepthmapReceiveNode.h"
#include "../lib/pipeline/nodes/NetworkReceiveNode.h"
#include "../lib/pipeline/nodes/RemoveBackgroundNode.h"
#include "../lib/pipeline/nodes/RemoveBlobsNode.h"
#include "../lib/rendering/pipelines.h"

int main() {
    auto engine = new glengine::Engine("Point Visualizer", int2(1280, 720));
    engine->SetAllowNonFocusedPawnInput(true);
    auto renderer = engine->GetRenderer();
    addPointmapperPipelines(renderer);

    auto pipeline = new pointmapper::pipeline::PointmapperPipeline(renderer->GetDevice(), wgpuDeviceGetQueue(renderer->GetDevice()));
    auto cloud = pipeline->CreateNode<pointmapper::pipeline::CreatePointCloudNode>();

    auto input = pipeline->CreateRoot<DepthmapReceiveNode>("127.0.0.1", 6767);
    cloud->camera_params->Connect(input->cameraParams);
    cloud->depth_map->Connect(input->depth);
    cloud->frameData->Connect(input->frameData);

    pipeline->Build();
    printf("Pipeline built!");

    auto actor = engine->SpawnActor<TestActor>();

    actor->SetNode(cloud->cloud);

    while (true) {
        glfwPollEvents();
        pipeline->Process();
        engine->Update();
        engine->Render();
    }
}
