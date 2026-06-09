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
    auto kDev = new Kinect2Device();
    auto rDev = new RealsenseDevice();

    //auto transform = registerDevices(rDev, kDev);

    //std::cout << transform << std::endl;

    /*mat4 transform = {
        0.966067, 0.242103, -0.0900153, 0,
        -0.205958, 0.932338, 0.297202, 0,
        0.155878, -0.268577, 0.950563, 0,
        0.000357822, 0.0639697, -0.194791, 1
    };*/

    std::set_terminate(my_terminate_handler);
    auto engine = new glengine::Engine("Pointmapper Demo", int2(1280, 720));
    engine->SetAllowNonFocusedPawnInput(true);
    addPointmapperPipelines(engine->GetRenderer());

    auto kinect = engine->SpawnActor<TestActor>();
    kinect->GetTransform()->SetPosition({-8, 0, 5});
    kinect->SetDevice(new Kinect2Device());

    auto realsense = engine->SpawnActor<TestActor>();
    realsense->GetTransform()->SetPosition({8, 0, 5});
    realsense->SetDevice(rDev);
    //realsense->SetRegistration(transform);

    engine->MainLoop();
}
