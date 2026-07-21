//
// Created by Kyle Smith on 2026-06-15.
//

#include "pointmapper/pipeline/GPUTexture.h"

namespace pointmapper::pipeline {

    GPUTexture::GPUTexture(WGPUTexture texture, WGPUTextureFormat format, unsigned int width, unsigned int height) {
        this->texture = texture;
        this->view = wgpuTextureCreateView(texture, nullptr);
        this->format = format;
        this->width = width;
        this->height = height;
    }

    GPUTexture::GPUTexture(const GPUTexture& other) {
        this->texture = other.texture;
        this->view = other.view;
        this->format = other.format;
        this->width = other.width;
        this->height = other.height;

        wgpuTextureAddRef(texture);
        wgpuTextureViewAddRef(view);
    }

    GPUTexture::~GPUTexture() {
        wgpuTextureRelease(texture);
        wgpuTextureViewRelease(view);
    }

    WGPUTextureFormat GPUTexture::GetFormat() const {
        return format;
    }

    unsigned int GPUTexture::GetWidth() const {
        return width;
    }

    unsigned int GPUTexture::GetHeight() const {
        return height;
    }

    GPUTexture::operator WGPUTextureView() const {
        return view;
    }

    GPUTexture::operator WGPUTexture() const {
        return texture;
    }

}
