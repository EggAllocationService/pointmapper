//
// Created by Kyle Smith on 2026-06-08.
//
#pragma once
#ifdef POINTMAPPER_USE_GLENGINE_MATH
#include "Matrix.h"
#else
#include "../PointmapperMath.h"
#endif
#include "../DepthDevice.h"

mat4 registerDevices(DepthDevice *source, DepthDevice *target);
