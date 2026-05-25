//
// Created by Kyle Smith on 2026-05-22.
//
#pragma once
#include "background/BackgroundProcessor.h"
#include "rendering/CloudRenderer.h"

/// Allows related objects to share GPU-side resources
class PointmapperPipeline {
public:
    PointmapperPipeline();

    ~PointmapperPipeline();

    std::shared_ptr<BackgroundProcessor> GetBackgroundProcessor();
    std::shared_ptr<CloudRenderer> GetCloudRenderer();

    void resize(CameraParams params);

private:
    void* device;
    void* queue;

    void* pointBuffer;
    void* infoBuffer;

    std::shared_ptr<BackgroundProcessor> backgroundProcessor;
    std::shared_ptr<CloudRenderer> cloudRenderer;
};
