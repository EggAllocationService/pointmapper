//
// Created by Kyle Smith on 2026-06-19.
//
#include "net.h"

template<>
uint32_t pointmapper::pipeline::net::reverse<uint32_t>(uint32_t val) {
    return ((val & 0xFF000000) >> 24) |
           ((val & 0x00FF0000) >>  8) |
           ((val & 0x0000FF00) <<  8) |
           ((val & 0x000000FF) << 24);
}
