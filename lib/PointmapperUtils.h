//
// Created by Kyle Smith on 2026-05-22.
//
#pragma once
#include "background/BackgroundProcessor.h"

class PointmapperUtils {
public:
    PointmapperUtils();

    ~PointmapperUtils();

    std::unique_ptr<BackgroundProcessor> GetBackgroundProcessor();

private:
    void* device;
    void* queue;
};
