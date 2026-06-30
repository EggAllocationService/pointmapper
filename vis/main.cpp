//
// Created by Kyle Smith on 2026-06-19.
//


#include "Engine.h"
#include "TestActor.h"
#include "../lib/kinect2/Kinect2Device.h"
#include "../lib/pipeline/PointmapperPipeline.h"
#include "../lib/pipeline/nodes/CpuToGpuCopyNode.h"
#include "../lib/pipeline/nodes/DepthCameraNode.h"
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
    auto cam = pipeline->CreateRoot<pointmapper::pipeline::DepthCameraNode>(new Kinect2Device());

    auto mask = pipeline->CreateNode<pointmapper::pipeline::RemoveBackgroundNode>();
    //auto blobs = pipeline->CreateNode<pointmapper::pipeline::RemoveBlobsNode>();

    auto cloud = pipeline->CreateNode<pointmapper::pipeline::CreatePointCloudNode>();
    cloud->camera_params->Connect(cam->params);
    cloud->color->Connect(cam->color);
    cloud->frameData->Connect(cam->frameData);

    mask->inputDepthMap->Connect(cam->depth);
    mask->camera_params->Connect(cam->params);
    mask->frameData->Connect(cam->frameData);

    //blobs->inputDepthMap->Connect(cam->depth);
    //blobs->camera_params->Connect(cam->params);
    //blobs->frameData->Connect(cam->frameData);

    cloud->depth_map->Connect(mask->depthMap);

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
