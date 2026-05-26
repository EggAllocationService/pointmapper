//
// Created by Kyle Smith on 2026-05-22.
//

#pragma once
#include <memory>

#include "Matrix.h"
#include "Transform.h"
#include "../background/BackgroundProcessor.h"
#include "GLFW/glfw3.h"


class BackgroundProcessor;

struct CloudUniforms {
    mat4 worldProjection;
};

class CloudRenderer {
public:
    CloudRenderer(std::shared_ptr<BackgroundProcessor> backgroundProcessor, WGPUInstance instance, WGPUAdapter adapter, WGPUDevice device, WGPUBuffer infoBuffer);

    // Performs the render loop once
    void spinOnce();
    void resize(WGPUBuffer pointsBuffer);
    Transform cameraTransform;
private:
    void clear(WGPUTextureView textureView, WGPUCommandEncoder encoder);
    void createPipelines();
    std::shared_ptr<BackgroundProcessor> backgroundProcessor;
    GLFWwindow* window;
    WGPUDevice device;
    WGPUQueue queue;
    WGPUSurface surface;
    WGPUSurfaceConfiguration surfaceConfig;
    WGPUTexture depthTexture;
    WGPUTextureView depthTextureView;
    WGPUShaderModule shaderModule;
    WGPUBuffer infoBuffer;

    WGPURenderPipeline pointsPipeline;

    WGPUBuffer cubeModel;
    WGPUBuffer uniformBuffer;

    WGPUBindGroup bindGroups[2];

    CloudUniforms uniforms;
};
