//
// Created by Kyle Smith on 2026-05-22.
//

#include "CloudRenderer.h"
#include "glfw3webgpu.h"
#include "cube.h"
#include "Matrix.h"
#include "GLMath.h"

const char shaders[] = {
#embed "shaders.wgsl"
};




CloudRenderer::CloudRenderer(std::shared_ptr<BackgroundProcessor> backgroundProcessor, WGPUInstance instance, WGPUAdapter adapter, WGPUDevice device, WGPUBuffer infoBuffer) {
    this->backgroundProcessor = backgroundProcessor;
    this->device = device;
    this->infoBuffer = infoBuffer;
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

    createPipelines();

    auto modelDesc = WGPUBufferDescriptor {
        .nextInChain = nullptr,
        .label = {},
        .usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex,
        .size = sizeof(float) * 36 * 4,
        .mappedAtCreation = false
    };
    this->cubeModel = wgpuDeviceCreateBuffer(device, &modelDesc);
    wgpuQueueWriteBuffer(queue, cubeModel, 0, kCubeVertices, sizeof(float) * 36 * 4);
    wgpuQueueSubmit(queue, 0, nullptr);

    auto uniformDesc = WGPUBufferDescriptor {
        .nextInChain = nullptr,
        .label = {},
        .usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform,
        .size = sizeof(CloudUniforms),
        .mappedAtCreation = false
    };
    this->uniformBuffer = wgpuDeviceCreateBuffer(device, &uniformDesc);

    auto uniformGroupDesc = WGPU_BIND_GROUP_ENTRY_INIT;
    uniformGroupDesc.buffer = uniformBuffer;
    auto bindGroupDesc = WGPU_BIND_GROUP_DESCRIPTOR_INIT;
    bindGroupDesc.layout = wgpuRenderPipelineGetBindGroupLayout(pointsPipeline, 0);
    bindGroupDesc.entries = &uniformGroupDesc;
    bindGroupDesc.entryCount = 1;

    bindGroups[0] = wgpuDeviceCreateBindGroup(device, &bindGroupDesc);
}

void CloudRenderer::spinOnce() {
    if (glfwWindowShouldClose(window)) {
        return;
    }

    auto viewMat = glengine::math::viewMatrix(cameraTransform.GetAbsoluteMatrix());
    auto projMat = glengine::math::perspectiveMatrix(90, 640.f / 480.f, 0.1, 100);

    uniforms.worldProjection = projMat * viewMat;
    // copy uniforms
    wgpuQueueWriteBuffer(queue, uniformBuffer, 0, &uniforms, sizeof(CloudUniforms));
    wgpuQueueSubmit(queue, 0, nullptr);

    auto encoder = wgpuDeviceCreateCommandEncoder(device, nullptr);
    WGPUSurfaceTexture texture;
    wgpuSurfaceGetCurrentTexture(surface, &texture);
    auto view = wgpuTextureCreateView(texture.texture, nullptr);

    clear(view, encoder);

    auto descriptor = WGPU_RENDER_PASS_DESCRIPTOR_INIT;
    auto attachment = WGPURenderPassDepthStencilAttachment {
        .nextInChain = nullptr,
        .view = depthTextureView,
        .depthLoadOp = WGPULoadOp_Load,
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
        .loadOp = WGPULoadOp_Load,
        .storeOp = WGPUStoreOp_Store,
        .clearValue = WGPUColor(0, 0, 0, 1)
    };
    descriptor.depthStencilAttachment = &attachment;
    descriptor.colorAttachmentCount = 1;
    descriptor.colorAttachments = &colorAttachment;
    auto pointsPass = wgpuCommandEncoderBeginRenderPass(encoder, &descriptor);
    wgpuRenderPassEncoderSetPipeline(pointsPass, pointsPipeline);
    wgpuRenderPassEncoderSetBindGroup(pointsPass, 0, bindGroups[0], 0, nullptr);
    wgpuRenderPassEncoderSetBindGroup(pointsPass, 1, bindGroups[1], 0, nullptr);
    wgpuRenderPassEncoderSetVertexBuffer(pointsPass, 0, cubeModel, 0, sizeof(float) * 36 * 4);
    //wgpuRenderPassEncoderDrawIndirect(pointsPass, infoBuffer, 0);
    wgpuRenderPassEncoderDraw(pointsPass, 36, 1, 0, 0);
    wgpuRenderPassEncoderEnd(pointsPass);

    auto bundle = wgpuCommandEncoderFinish(encoder, nullptr);
    wgpuQueueSubmit(queue, 1, &bundle);

    wgpuTextureViewRelease(view);
    wgpuSurfacePresent(surface);
    wgpuTextureRelease(texture.texture);
    glfwPollEvents();
}

void CloudRenderer::resize(WGPUBuffer pointsBuffer) {
    if (this->bindGroups[1] != nullptr) {
        wgpuBindGroupRelease(this->bindGroups[1]);
    }

    auto pointsGroupDesc = WGPU_BIND_GROUP_ENTRY_INIT;
    pointsGroupDesc.buffer = pointsBuffer;
    auto bindGroupDesc = WGPU_BIND_GROUP_DESCRIPTOR_INIT;
    bindGroupDesc.layout = wgpuRenderPipelineGetBindGroupLayout(pointsPipeline, 1);
    bindGroupDesc.entries = &pointsGroupDesc;
    bindGroupDesc.entryCount = 1;

    bindGroups[1] = wgpuDeviceCreateBindGroup(device, &bindGroupDesc);
}

void CloudRenderer::clear(WGPUTextureView view, WGPUCommandEncoder encoder) {

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
}

void CloudRenderer::createPipelines() {
    // point cloud render pipeline

    auto fragTarget = WGPU_COLOR_TARGET_STATE_INIT;
    fragTarget.format = surfaceConfig.format;
    fragTarget.writeMask = WGPUColorWriteMask_All;

    auto fragDesc = WGPU_FRAGMENT_STATE_INIT;
    fragDesc.module = shaderModule;
    fragDesc.targetCount = 1;
    fragDesc.targets = &fragTarget;

    auto attr = WGPU_VERTEX_ATTRIBUTE_INIT;
    attr.format = WGPUVertexFormat_Float32x4;
    attr.shaderLocation = 0;
    attr.offset = 0;

    auto vtx = WGPU_VERTEX_BUFFER_LAYOUT_INIT;
    vtx.arrayStride = sizeof(float) * 4;
    vtx.attributeCount = 1;
    vtx.attributes = &attr;
    vtx.stepMode = WGPUVertexStepMode_Vertex;

    WGPUDepthStencilState depthState = {
        .nextInChain = nullptr,
        .format = WGPUTextureFormat_Depth24Plus,
        .depthWriteEnabled = WGPUOptionalBool_True,
        .depthCompare = WGPUCompareFunction_Less,
        .stencilFront = WGPU_STENCIL_FACE_STATE_INIT,
        .stencilBack = WGPU_STENCIL_FACE_STATE_INIT,
        .stencilReadMask = 0,
        .stencilWriteMask = 0,
        .depthBias = 0,
        .depthBiasSlopeScale = 0,
        .depthBiasClamp = 0
    };

    auto desc = WGPURenderPipelineDescriptor {
        .nextInChain = nullptr,
        .label = {},
        .layout = nullptr,
        .vertex = {
            .nextInChain = nullptr,
            .module = shaderModule,
            .entryPoint = {
                .data = "vs",
                .length = 2
            },
            .constantCount = 0,
            .constants = nullptr,
            .bufferCount = 1,
            .buffers = &vtx
        },
        .primitive = {
            .nextInChain = nullptr,
            .topology = WGPUPrimitiveTopology_TriangleList,
            .stripIndexFormat = WGPUIndexFormat_Undefined,
            .frontFace = WGPUFrontFace_CCW,
            .cullMode = WGPUCullMode_Back,
            .unclippedDepth = false,
        },
        .depthStencil = &depthState,
        .multisample = {
            .nextInChain = nullptr,
            .count = 1,
            .mask = 0xFFFFFFFF,
            .alphaToCoverageEnabled = true
        },
        .fragment = &fragDesc
    };

    pointsPipeline = wgpuDeviceCreateRenderPipeline(device, &desc);
}
