//
// Created by Kyle Smith on 2026-05-22.
//

#include <iostream>
#include <exception>
#include <execinfo.h>

#include "Engine.h"
#include "../vis/TestActor.h"
#include "../lib/pipeline/PointmapperPipeline.h"
#include "../lib/kinect2/Kinect2Device.h"
#include "../lib/pipeline/nodes/CreatePointCloudNode.h"
#include "../lib/pipeline/nodes/DepthCameraNode.h"
#include "../lib/pipeline/nodes/RemoveBackgroundNode.h"
#include "../lib/pipeline/nodes/RemoveBlobsNode.h"
#include "../lib/pipeline/nodes/GpuToCpuCopyNode.h"
#include "../lib/rendering/pipelines.h"

#include "../lib/pipeline/nodes/NetworkSendNode.h"

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
    enet_initialize();

    auto pipeline = new pointmapper::pipeline::PointmapperPipeline();

    auto cam = pipeline->CreateRoot<pointmapper::pipeline::DepthCameraNode>(new Kinect2Device());

    auto mask = pipeline->CreateNode<pointmapper::pipeline::RemoveBackgroundNode>();
    auto blobs = pipeline->CreateNode<pointmapper::pipeline::RemoveBlobsNode>();

    auto cloud = pipeline->CreateNode<pointmapper::pipeline::CreatePointCloudNode>();
    cloud->camera_params->Connect(cam->params);
    cloud->color->Connect(cam->color);
    cloud->frameData->Connect(cam->frameData);

    mask->inputDepthMap->Connect(cam->depth);
    mask->camera_params->Connect(cam->params);

    blobs->inputDepthMap->Connect(mask->depthMap);
    blobs->camera_params->Connect(cam->params);
    blobs->frameData->Connect(cam->frameData);

    cloud->depth_map->Connect(blobs->depthMap);

    auto cpuCopy = pipeline->CreateNode<pointmapper::pipeline::GpuToCpuCopyNode>();
    cpuCopy->cloud->Connect(cloud->cloud);

    auto output = pipeline->CreateNode<pointmapper::pipeline::NetworkSendNode>();
    output->cloud->Connect(cpuCopy->cpuCloud);

    pipeline->Build();

    printf("Pipeline built!!!\n");

    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        pipeline->Process();
    }
}
