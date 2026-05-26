//
// Created by Kyle Smith on 2026-05-22.
//


#include <iostream>

#include "../lib/background/BackgroundProcessor.h"
#include "../lib/realsense/RealsenseDevice.h"

#include <exception>
#include <iostream>
#include <execinfo.h> // For backtrace() on Linux/macOS

#include "../lib/PointmapperPipeline.h"
#include <pcl/visualization/cloud_viewer.h>
#include "../lib/kinect2/Kinect2Device.h"

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
    auto utils = new PointmapperPipeline();
    auto device = new Kinect2Device();
    auto processor = utils->GetBackgroundProcessor();
    auto renderer = utils->GetCloudRenderer();
    utils->resize(device->GetCameraParameters());

    std::vector<PointXYZRGB> points;
    pcl::visualization::PCLVisualizer viewer ("Simple Cloud Viewer");
    auto cloud = std::make_shared<pcl::PointCloud<pcl::PointXYZRGB>>();
    viewer.addPointCloud(cloud, "cloud");
    viewer.setBackgroundColor(0, 0, 0);
    viewer.initCameraParameters();

    while (true) {
        auto frame = device->GetNextFrame();
        if (frame == nullptr) { continue;}
        processor->processFrame(frame, points);

        if (points.size() != cloud->points.size()) {
            cloud->points.resize(points.size());
        }
        for (int i = 0; i < points.size(); i++) {
            cloud->points[i].x = points[i].x;
            cloud->points[i].y = points[i].y;
            cloud->points[i].z = points[i].z;

            cloud->points[i].r = 255;
            cloud->points[i].g = 255;
            cloud->points[i].b = 255;
        }

        viewer.removePointCloud("cloud");
        viewer.addPointCloud(cloud, "cloud");
        viewer.setPointCloudRenderingProperties(pcl::visualization::PCL_VISUALIZER_POINT_SIZE, 10);
        viewer.spinOnce();

    }
}
