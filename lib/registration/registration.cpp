//
// Created by Kyle Smith on 2026-06-08.
//

#include "pointmapper/registration/registration.h"

#include <pcl-1.15/pcl/point_cloud.h>
#include <pcl/visualization/pcl_visualizer.h>
#include <pcl-1.15/pcl/impl/point_types.hpp>
#include <pcl-1.15/pcl/registration/ia_ransac.h>
#include <pcl/filters/statistical_outlier_removal.h>


std::shared_ptr<pcl::PointCloud<pcl::PointXYZ>> getCloud(Frame& frame, CameraParams& params) {
    auto cloud = std::make_shared<pcl::PointCloud<pcl::PointXYZ>>();
    cloud->reserve(params.width * params.height);
    auto depth = frame.GetDepth();
    auto units = frame.GetDepthUnits();
    auto scale = frame.GetAxisScale();


    for (int y = 0; y < params.height; y++) {
        for (int x = 0; x < params.width; x++) {
            int idx = (y * params.width) + x;
            pcl::PointXYZ point;

            float d = depth[idx] * units;
            point.x = (x - params.cx) * (d/params.fx) * scale.x;
            point.y = (y - params.cy) * (d/params.fy) * scale.y;
            point.z = d * scale.z;
            cloud->push_back(point);
        }
    }

    return std::move(cloud);
}

mat4 registerDevices(DepthDevice *source, DepthDevice *target) {
    for (int i = 0; i < 10; i++) {
        source->GetNextFrame();
        target->GetNextFrame();
    }

    auto srcFrame = source->GetNextFrame();
    auto targetFrame = target->GetNextFrame();
    auto srcParams = source->GetCameraParameters();
    auto targetParams = target->GetCameraParameters();

    auto dstFiltered = std::make_shared<pcl::PointCloud<pcl::PointXYZ>>();
    auto srcFiltered = std::make_shared<pcl::PointCloud<pcl::PointXYZ>>();
    {
        auto srcCloud = getCloud(*srcFrame, srcParams);
        auto dstCloud = getCloud(*targetFrame, targetParams);
        printf("Filtering src cloud");
        {
            pcl::StatisticalOutlierRemoval<pcl::PointXYZ> sor;
            sor.setInputCloud(srcCloud);
            sor.setMeanK(50);
            sor.setStddevMulThresh(1.0);
            sor.filter(*srcFiltered);
        }

        printf("filtering dst cloud");
        {
            pcl::StatisticalOutlierRemoval<pcl::PointXYZ> sor;
            sor.setInputCloud(dstCloud);
            sor.setMeanK(50);
            sor.setStddevMulThresh(1.0);
            sor.filter(*dstFiltered);
        }
    }

    pcl::visualization::PCLVisualizer viewer ("Simple Cloud Viewer");
    viewer.setBackgroundColor(0, 0, 0);
    viewer.initCameraParameters();

    viewer.addPointCloud(dstFiltered, "target");
    viewer.addPointCloud(srcFiltered, "src");

    viewer.setPointCloudRenderingProperties(pcl::visualization::RenderingProperties::PCL_VISUALIZER_COLOR, 1, 0, 0, "result");
    viewer.setPointCloudRenderingProperties(pcl::visualization::RenderingProperties::PCL_VISUALIZER_COLOR, 0, 0, 1, "target");
    viewer.setPointCloudRenderingProperties(pcl::visualization::RenderingProperties::PCL_VISUALIZER_COLOR, 0, 1, 0, "src");



    while (!viewer.wasStopped()) {
        viewer.spinOnce();
    }

    return mat4::identity();
}
