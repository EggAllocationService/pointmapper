//
// Minimal math types used when the rendering engine (glengine) is not available.
//
#pragma once

struct float4 {
    float x, y, z, w;
};

struct mat4 {
    float m[16];

    static mat4 identity() {
        mat4 result{};
        result.m[0] = 1.0f;
        result.m[5] = 1.0f;
        result.m[10] = 1.0f;
        result.m[15] = 1.0f;
        return result;
    }
};
