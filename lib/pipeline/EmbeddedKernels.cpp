//
// Created by Kyle Smith on 2026-06-15.
//

#include "EmbeddedKernels.h"

namespace pointmapper::pipeline {
    const char embeddedKernels[] = {
    #embed "../rendering/kernels.wgsl"
    , '\0'
    };
    const std::size_t embeddedKernelsLength = sizeof(embeddedKernels) - 1;
}
