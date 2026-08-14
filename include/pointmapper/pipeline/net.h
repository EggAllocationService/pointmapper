#pragma once
#include <concepts>
#include <bit>

#include "../common.h"

#define NET_MAX_PACKET_SIZE 1000ul
#define NET_RECEIVE_BUFFER_COUNT 5

namespace pointmapper::pipeline::net {
    template<typename T>
    T reverse(T val) {
        static_assert(false, "No implementation to swap T");
        return 0;
    }

    template<>
    uint32_t reverse<uint32_t>(uint32_t val);

    template<>
    CameraParams reverse<CameraParams>(CameraParams val);

    template <typename T>
    constexpr T to_network_order(T value) {
        if constexpr (std::endian::native == std::endian::little) {
            return reverse(value);
        }
        return value;
    }

    struct NetHeader {
        char magic[3];
        char kind;
        uint32_t cloud_id;
    };

    struct NetInfoPacket {
        uint32_t maxPoints;
    };

    struct DepthMapInfoPacket {
        uint32_t width;
        uint32_t height;
        CameraParams cameraParams;
    };

}