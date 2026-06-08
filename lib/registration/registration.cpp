//
// Created by Kyle Smith on 2026-06-08.
//

#include "registration.h"

#include <pcl-1.15/pcl/point_cloud.h>
#include <pcl-1.15/pcl/impl/point_types.hpp>
#include <pcl-1.15/pcl/registration/icp.h>


std::shared_ptr<pcl::PointCloud<pcl::PointXYZ>> getCloud(Frame& frame, CameraParams& params) {
    auto cloud = std::make_shared<pcl::PointCloud<pcl::PointXYZ>>();
    cloud->reserve(params.width * params.height);
    auto depth = frame.GetDepth();
    auto units = frame.GetDepthUnits();
    for (int y = 0; y < params.height; y++) {
        for (int x = 0; x < params.width; x++) {
            int idx = (y * params.width) + x;
            pcl::PointXYZ point;

            float d = depth[idx] * units;
            point.x = (x - params.cx) * (d/params.fx);
            point.y = (y - params.cy) * (d/params.fy);
            point.z = d;
            cloud->push_back(point);
        }
    }

    return std::move(cloud);
}

mat4 registerDevices(DepthDevice *source, DepthDevice *target) {
    auto srcFrame = source->GetNextFrame();
    auto targetFrame = target->GetNextFrame();

    auto srcParams = source->GetCameraParameters();
    auto targetParams = target->GetCameraParameters();

    auto srcCloud = getCloud(*srcFrame, srcParams);
    auto targetCloud = getCloud(*targetFrame, targetParams);

    pcl::IterativeClosestPoint<pcl::PointXYZ, pcl::PointXYZ> icp;

    icp.setInputSource(srcCloud);
    icp.setInputTarget(targetCloud);

    auto result = std::make_shared<pcl::PointCloud<pcl::PointXYZ>>();

    pcl::PointCloud<pcl::PointXYZ> points;

    icp.align(points);

    auto transform = icp.getFinalTransformation();

    return *reinterpret_cast<mat4*>(&transform);
}
