//
// Created by Kyle Smith on 2026-06-08.
//

#include "registration.h"

#include <pcl-1.15/pcl/point_cloud.h>
#include <pcl/visualization/pcl_visualizer.h>
#include <pcl-1.15/pcl/impl/point_types.hpp>
#include <pcl/keypoints/sift_keypoint.h>
#include <pcl/visualization/range_image_visualizer.h>
#include <pcl/range_image/range_image.h>
#include <pcl/features/range_image_border_extractor.h>
#include <pcl/keypoints/narf_keypoint.h>
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

    // create range image
    auto srcRange = std::make_shared<pcl::RangeImage>();
    auto dstRange = std::make_shared<pcl::RangeImage>();
    {
        {
            printf("Creating src range image");
            srcRange->createFromPointCloud(*srcFiltered, pcl::deg2rad(0.1), pcl::deg2rad(120.0), pcl::deg2rad(120.0), Eigen::Affine3f::Identity());
        }

        {
            printf("Creating dst range image");
            dstRange->createFromPointCloud(*dstFiltered, pcl::deg2rad(0.1), pcl::deg2rad(120.0), pcl::deg2rad(120.0), Eigen::Affine3f::Identity());
        }
    }

    pcl::PointCloud<int> keypointIndices;
    // extract keypoints
    {
        {
            printf("extracting keypoints from src");
            pcl::NarfKeypoint narf;
            auto extractor = new pcl::RangeImageBorderExtractor(srcRange.get());
            narf.setRangeImageBorderExtractor(extractor);
            narf.getParameters().support_size = 0.2;


            narf.compute(keypointIndices);
        }
    }

    auto keypointsPtr = std::make_shared<pcl::PointCloud<pcl::PointXYZ>>();
    auto& keypoints = *keypointsPtr;
    keypoints.resize(keypointIndices.size());
    for (int i = 0; i < keypointIndices.size(); i++) {
        auto x = srcRange->operator[](i);
        keypoints[i].getVector3fMap() = x.getVector3fMap();
    }
    printf("\n======= FOUND %lu KEYPOINTS\n", keypointIndices.size());

    pcl::visualization::PCLVisualizer viewer ("Simple Cloud Viewer");
    viewer.setBackgroundColor(0, 0, 0);
    viewer.initCameraParameters();

    //viewer.addPointCloud(dstFiltered, "target");
    viewer.addPointCloud(keypointsPtr, "src");

    pcl::visualization::RangeImageVisualizer rangeVis;
    rangeVis.showRangeImage(*srcRange);
    //rangeVis.showRangeImage(*dstRange);

    viewer.setPointCloudRenderingProperties(pcl::visualization::RenderingProperties::PCL_VISUALIZER_COLOR, 1, 0, 0, "result");
    viewer.setPointCloudRenderingProperties(pcl::visualization::RenderingProperties::PCL_VISUALIZER_COLOR, 0, 0, 1, "target");
    viewer.setPointCloudRenderingProperties(pcl::visualization::PCL_VISUALIZER_POINT_SIZE, 10, "src");
    viewer.setPointCloudRenderingProperties(pcl::visualization::RenderingProperties::PCL_VISUALIZER_COLOR, 0, 1, 0, "src");



    while (!viewer.wasStopped()) {
        viewer.spinOnce();
        rangeVis.spinOnce();
        pcl_sleep(0.01);
    }

    return mat4::identity();
}
