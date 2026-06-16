//
// Created by Kyle Smith on 2026-05-22.
//

#include <iostream>
#include <exception>
#include <execinfo.h>

#include "Engine.h"
#include "TestActor.h"
#include "../lib/pipeline/PointmapperPipeline.h"
#include "../lib/kinect2/Kinect2Device.h"
#include "../lib/pipeline/nodes/CreatePointCloudNode.h"
#include "../lib/pipeline/nodes/DepthCameraNode.h"
#include "../lib/pipeline/nodes/RemoveBackgroundNode.h"
#include "../lib/pipeline/nodes/RemoveBlobsNode.h"
#include "../lib/pipeline/nodes/GpuToCpuCopyNode.h"
#include "../lib/rendering/pipelines.h"

void my_terminate_handler() {
    void* array[10];
    size_t size = backtrace(array, 10);
    std::cerr << "Uncaught exception! Backtrace:\n";
    auto trace = backtrace_symbols(array, size);
    for (int i = 0; i < size; i++) {
        printf("\t%s\n", trace[i]);
    }
    std::abort();
}

int main() {
    auto engine = new glengine::Engine("Test Window", int2(1280, 720));
    engine->SetAllowNonFocusedPawnInput(true);
    auto renderer = engine->GetRenderer();
    addPointmapperPipelines(renderer);
    auto pipeline = new pointmapper::pipeline::PointmapperPipeline(renderer->GetDevice(), wgpuDeviceGetQueue(renderer->GetDevice()));
    auto cam = pipeline->CreateRoot<pointmapper::pipeline::DepthCameraNode>(new Kinect2Device());

    auto mask = pipeline->CreateNode<pointmapper::pipeline::RemoveBackgroundNode>();
    auto blobs = pipeline->CreateNode<pointmapper::pipeline::RemoveBlobsNode>();

    auto cloud = pipeline->CreateNode<pointmapper::pipeline::CreatePointCloudNode>();
    cloud->camera_params->Connect(cam->params);
    cloud->color->Connect(cam->color);
    cloud->frameData->Connect(cam->frameData);

    mask->inputDepthMap->Connect(cam->depth);
    mask->camera_params->Connect(cam->params);
    mask->frameData->Connect(cam->frameData);

    blobs->inputDepthMap->Connect(mask->depthMap);
    blobs->camera_params->Connect(cam->params);
    blobs->frameData->Connect(cam->frameData);

    cloud->depth_map->Connect(blobs->depthMap);

    auto cpuCopy = pipeline->CreateNode<pointmapper::pipeline::GpuToCpuCopyNode>();
    cpuCopy->cloud->Connect(cloud->cloud);

    pipeline->Build();

    printf("Pipeline built!!!\n");

    auto test = engine->SpawnActor<TestActor>();

    test->SetNode(cloud);

    while (true) {
        glfwPollEvents();
        pipeline->Process();

        engine->Update();
        engine->Render();
    }
}
