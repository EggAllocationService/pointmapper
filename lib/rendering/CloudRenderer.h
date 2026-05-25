//
// Created by Kyle Smith on 2026-05-22.
//

#pragma once
#include <memory>
#include "../background/BackgroundProcessor.h"
#include "GLFW/glfw3.h"


class BackgroundProcessor;

class CloudRenderer {
public:
    CloudRenderer(std::shared_ptr<BackgroundProcessor> backgroundProcessor, WGPUInstance instance, WGPUAdapter adapter, WGPUDevice device);

private:
    std::shared_ptr<BackgroundProcessor> backgroundProcessor;
    GLFWwindow* window;
    WGPUDevice device;
    WGPUQueue queue;
    WGPUSurface surface;
    WGPUSurfaceConfiguration surfaceConfig;
};
