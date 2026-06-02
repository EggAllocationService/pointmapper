//
// Created by Kyle Smith on 2026-05-22.
//


#include <iostream>
#include "../lib/realsense/RealsenseDevice.h"

#include <exception>
#include <iostream>
#include <execinfo.h> // For backtrace() on Linux/macOS

#include "../lib/PointmapperPipeline.h"
#include <pcl/visualization/cloud_viewer.h>

#include "Engine.h"
#include "TestActor.h"
#include "../lib/kinect2/Kinect2Device.h"
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
    std::set_terminate(my_terminate_handler);
    auto engine = new glengine::Engine("Pointmapper Demo", int2(1280, 720));
    engine->SetAllowNonFocusedPawnInput(true);
    addPointmapperPipelines(engine->GetRenderer());

    auto kinect = engine->SpawnActor<TestActor>();
    kinect->GetTransform()->SetPosition({-8, 0, 5});
    kinect->SetDevice(new Kinect2Device());

    auto realsense = engine->SpawnActor<TestActor>();
    realsense->GetTransform()->SetPosition({8, 0, 5});
    realsense->SetDevice(new RealsenseDevice());

    engine->MainLoop();
}
