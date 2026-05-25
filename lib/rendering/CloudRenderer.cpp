//
// Created by Kyle Smith on 2026-05-22.
//

#include "CloudRenderer.h"
#include "glfw3webgpu.h"

const char shaders[] = {
#embed "shaders.wgsl"
};


CloudRenderer::CloudRenderer(std::shared_ptr<BackgroundProcessor> backgroundProcessor, WGPUInstance instance, WGPUAdapter adapter, WGPUDevice device) {
    this->backgroundProcessor = backgroundProcessor;
    this->device = device;
    this->queue = wgpuDeviceGetQueue(device);

    glfwInit();

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
    surfaceConfig.format = preferredFormat;
    surfaceConfig.viewFormats = &preferredFormat;
    surfaceConfig.viewFormatCount = 1;
    surfaceConfig.width = 640;
    surfaceConfig.height = 480;

    wgpuSurfaceConfigure(surface, &surfaceConfig);
    wgpuSurfaceCapabilitiesFreeMembers(surfaceCaps);

    auto depthDescriptor = WGPU_TEXTURE_DESCRIPTOR_INIT;
    depthDescriptor.format = WGPUTextureFormat_Depth24Plus;
    depthDescriptor.dimension = WGPUTextureDimension_2D;
    depthDescriptor.size = {
        .width = 640,
        .height = 480,
        .depthOrArrayLayers = 1
    };
    depthDescriptor.usage = WGPUTextureUsage_RenderAttachment;
    depthTexture = wgpuDeviceCreateTexture(device, &depthDescriptor);

    depthTextureView = wgpuTextureCreateView(depthTexture, nullptr);

    auto shaderSource = WGPU_SHADER_SOURCE_WGSL_INIT;
    shaderSource.code.data = shaders;
    shaderSource.code.length = sizeof(shaders);

    auto shaderDescriptor = WGPU_SHADER_MODULE_DESCRIPTOR_INIT;
    shaderDescriptor.nextInChain = &shaderSource.chain;
    this->shaderModule = wgpuDeviceCreateShaderModule(device, &shaderDescriptor);
}

void CloudRenderer::spinOnce() {
    if (glfwWindowShouldClose(window)) {
        return;
    }

    WGPUSurfaceTexture texture;
    wgpuSurfaceGetCurrentTexture(surface, &texture);
    auto view = wgpuTextureCreateView(texture.texture, nullptr);

    clear(view);


    wgpuTextureViewRelease(view);
    wgpuSurfacePresent(surface);
    wgpuTextureRelease(texture.texture);
    glfwPollEvents();
}

void CloudRenderer::clear(WGPUTextureView view) {


    auto encoder = wgpuDeviceCreateCommandEncoder(device, nullptr);
    auto descriptor = WGPU_RENDER_PASS_DESCRIPTOR_INIT;
    auto attachment = WGPURenderPassDepthStencilAttachment {
        .nextInChain = nullptr,
        .view = depthTextureView,
        .depthLoadOp = WGPULoadOp_Clear,
        .depthStoreOp = WGPUStoreOp_Store,
        .depthClearValue = 0.0,
        .depthReadOnly = false,
        .stencilLoadOp = WGPULoadOp_Clear,
        .stencilStoreOp = WGPUStoreOp_Store,
        .stencilClearValue = 0,
        .stencilReadOnly = false
    };
    auto colorAttachment = WGPURenderPassColorAttachment {
        .nextInChain = nullptr,
        .view = view,
        .depthSlice = WGPU_DEPTH_SLICE_UNDEFINED,
        .resolveTarget = nullptr,
        .loadOp = WGPULoadOp_Clear,
        .storeOp = WGPUStoreOp_Store,
        .clearValue = WGPUColor(0, 0, 0, 1)
    };
    descriptor.depthStencilAttachment = &attachment;
    descriptor.colorAttachmentCount = 1;
    descriptor.colorAttachments = &colorAttachment;

    auto pass = wgpuCommandEncoderBeginRenderPass(encoder, &descriptor);
    wgpuRenderPassEncoderEnd(pass);
    auto bundle = wgpuCommandEncoderFinish(encoder, nullptr);

    wgpuQueueSubmit(queue, 1, &bundle);
}





