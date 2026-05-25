//
// Created by Kyle Smith on 2026-05-22.
//

#pragma once
#include <memory>
#include "../background/BackgroundProcessor.h"


class BackgroundProcessor;

class CloudRenderer {
public:
    CloudRenderer(std::shared_ptr<BackgroundProcessor> backgroundProcessor);

private:
    std::shared_ptr<BackgroundProcessor> backgroundProcessor;
};
