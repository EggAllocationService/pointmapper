//
// Created by Kyle Smith on 2026-05-22.
//


#include <iostream>
#include "../lib/realsense/RealsenseDevice.h"

#include <exception>
#include <iostream>
#include <execinfo.h> // For backtrace() on Linux/macOS

#include "../lib/pipeline/PointmapperPipeline.h"
#include <pcl/visualization/cloud_viewer.h>

#include "Engine.h"
#include "TestActor.h"
#include "../lib/kinect2/Kinect2Device.h"
#include "../lib/pipeline/nodes/CreatePointCloudNode.h"
#include "../lib/pipeline/nodes/DepthCameraNode.h"
#include "../lib/rendering/pipelines.h"
#include "../lib/registration/registration.h"

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

    auto pipeline = new pointmapper::pipeline::PointmapperPipeline();

    auto cam = pipeline->CreateRoot<pointmapper::pipeline::DepthCameraNode>(new Kinect2Device());

    auto cloud = pipeline->CreateNode<pointmapper::pipeline::CreatePointCloudNode>();
    cloud->camera_params->Connect(cam->params);
    cloud->depth_map->Connect(cam->depth);

    pipeline->Build();

    printf("Pipeline built!!!");
}
