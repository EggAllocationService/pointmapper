//
// Created by Kyle Smith on 2026-05-22.
//

#include "CloudRenderer.h"
#include "glfw3webgpu.h"


CloudRenderer::CloudRenderer(std::shared_ptr<BackgroundProcessor> backgroundProcessor, WGPUInstance instance, WGPUAdapter adapter, WGPUDevice device) {
    this->backgroundProcessor = backgroundProcessor;
    this->device = device;
    this->queue = wgpuDeviceGetQueue(device);

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    window = glfwCreateWindow(640, 480, "Hello World!", nullptr, nullptr);
    surface = glfwCreateWindowWGPUSurface(instance, window);

    WGPUSurfaceCapabilities surfaceCaps;
    wgpuSurfaceGetCapabilities(surface, adapter, &surfaceCaps);

    auto preferredFormat = surfaceCaps.formats[0];
    surfaceConfig = WGPU_SURFACE_CONFIGURATION_INIT;
    surfaceConfig.device = device;
    surfaceConfig.usage = WGPUTextureUsage_RenderAttachment;
    surfaceConfig.presentMode = WGPUPresentMode_Fifo;
    surfaceConfig.viewFormats = &preferredFormat;
    surfaceConfig.viewFormatCount = 1;
    surfaceConfig.width = 640;
    surfaceConfig.height = 480;

    wgpuSurfaceConfigure(surface, &surfaceConfig);
}



