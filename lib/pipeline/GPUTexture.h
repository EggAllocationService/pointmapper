//
// Created by Kyle Smith on 2026-06-15.
//
#pragma once

#include <webgpu/wgpu.h>

namespace pointmapper::pipeline {

    class GPUTexture {
    public:
        GPUTexture(WGPUTexture texture, WGPUTextureFormat format, unsigned int width, unsigned int height);
        GPUTexture(const GPUTexture& other);
        ~GPUTexture();

        [[nodiscard]] WGPUTextureFormat GetFormat() const;
        [[nodiscard]] unsigned int GetWidth() const;
        [[nodiscard]] unsigned int GetHeight() const;

        operator WGPUTextureView() const;
        operator WGPUTexture() const;

    private:
        WGPUTexture texture;
        WGPUTextureView view;
        WGPUTextureFormat format;
        unsigned int width;
        unsigned int height;
    };

}
