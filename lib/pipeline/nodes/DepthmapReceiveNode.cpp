//
// Created by Kyle Smith on 2026-07-16.
//

#include "DepthmapReceiveNode.h"

#include "../PointmapperPipeline.h"

#include <cassert>
#include <cstring>

DepthmapReceiveNode::DepthmapReceiveNode(std::string_view remoteAddress, uint16_t port) {
    std::string remote(remoteAddress);
    ENetAddress address = {0};
    address.port = port;

    enet_address_set_host_ip_new(&address, remote.c_str());

    client = enet_host_create(
        nullptr,
        1,
        2,
        0,
        0
    );

    client->mtu = 60000;

    assert(client);

    server = enet_host_connect(client, &address, 2, 0);

    depth = CreateOutput<pointmapper::pipeline::GPUDepthMap>();
    cameraParams = CreateOutput<CameraParams>();
    frameData = CreateOutput<pointmapper::pipeline::FrameData>();

    **depth = pointmapper::pipeline::GPUDepthMap{};
    **cameraParams = CameraParams{};
    **frameData = pointmapper::pipeline::FrameData{};

    ProcessLazily = false;
}

DepthmapReceiveNode::~DepthmapReceiveNode() {
    if ((*depth)->buffer != nullptr) {
        wgpuBufferRelease((*depth)->buffer);
    }
    enet_host_destroy(client);
}

void DepthmapReceiveNode::Hydrate() {
    ENetEvent event;
    enet_host_service(client, &event, 7000);
    if (event.type != ENET_EVENT_TYPE_CONNECT) {
        printf("Failed to connect to depth server (%u)\n", event.type);
        return;
    }

    while (true) {
        enet_host_service(client, &event, 10);

        if (event.type == ENET_EVENT_TYPE_RECEIVE) {
            if (event.channelID != 1 || event.packet->dataLength != sizeof(pointmapper::pipeline::net::DepthMapInfoPacket)) {
                printf("WARN: receive packet during depth init which wasn't on channel 1!\n");
                continue;
            }
            auto info = reinterpret_cast<pointmapper::pipeline::net::DepthMapInfoPacket*>(event.packet->data);
            auto width = pointmapper::pipeline::net::to_network_order(info->width);
            auto height = pointmapper::pipeline::net::to_network_order(info->height);
            auto params = pointmapper::pipeline::net::to_network_order(info->cameraParams);

            **cameraParams = params;
            **frameData = pointmapper::pipeline::FrameData{};
            (*depth)->width = width;
            (*depth)->height = height;

            auto bufferSize = static_cast<unsigned int>(width * height * sizeof(float));
            depthBuffer.resize(static_cast<size_t>(width) * height);

            (*depth)->buffer = pointmapper::pipeline::PIPELINE->CreateBuffer(
                bufferSize,
                WGPUBufferUsage_CopyDst | WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc);

            depth->MarkReady();
            cameraParams->MarkReady();
            frameData->MarkReady();
            enet_packet_destroy(event.packet);
            break;
        }
    }

    field = zfp_field_alloc();
    zfp = zfp_stream_open(nullptr);
}

void DepthmapReceiveNode::Process(pointmapper::pipeline::PipelineBundle&) {
    while (true) {
        ENetEvent event;
        enet_host_service(client, &event, 0);

        if (event.type == ENET_EVENT_TYPE_RECEIVE && event.channelID == 0) {
            auto packet = event.packet;
            auto header = *reinterpret_cast<pointmapper::pipeline::net::NetHeader*>(packet->data);
            auto cloudId = header.cloud_id;

            if (cloudId <= lastPresentedCloudId) {
                enet_packet_destroy(packet);
                break;
            }

            if (packet->dataLength < sizeof(pointmapper::pipeline::net::NetHeader) + sizeof(pointmapper::pipeline::FrameData)) {
                printf("WARN: depth packet too small\n");
                enet_packet_destroy(packet);
                break;
            }

            auto frame = *reinterpret_cast<pointmapper::pipeline::FrameData*>(
                packet->data + sizeof(pointmapper::pipeline::net::NetHeader));

            size_t compressedSize = packet->dataLength
                - sizeof(pointmapper::pipeline::net::NetHeader)
                - sizeof(pointmapper::pipeline::FrameData);

            auto instream = stream_open(
                packet->data + sizeof(pointmapper::pipeline::net::NetHeader) + sizeof(pointmapper::pipeline::FrameData),
                compressedSize);
            zfp_stream_set_bit_stream(zfp, instream);
            zfp_stream_rewind(zfp);
            zfp_read_header(zfp, field, ZFP_HEADER_FULL);

            size_t dimensions[4];
            zfp_field_size(field, dimensions);
            size_t pointCount = dimensions[0] * dimensions[1];
            if (pointCount != depthBuffer.size()) {
                depthBuffer.resize(pointCount);
                (*depth)->width = static_cast<unsigned int>(dimensions[0]);
                (*depth)->height = static_cast<unsigned int>(dimensions[1]);
                if ((*depth)->buffer != nullptr) {
                    wgpuBufferRelease((*depth)->buffer);
                }
                (*depth)->buffer = pointmapper::pipeline::PIPELINE->CreateBuffer(
                    static_cast<unsigned int>(pointCount * sizeof(float)),
                    WGPUBufferUsage_CopyDst | WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc);
            }

            zfp_field_set_pointer(field, depthBuffer.data());
            zfp_decompress(zfp, field);
            stream_close(instream);
            enet_packet_destroy(packet);

            **frameData = frame;

            wgpuQueueWriteBuffer(pointmapper::pipeline::PIPELINE->GetQueue(), (*depth)->buffer, 0,
                                 depthBuffer.data(), depthBuffer.size() * sizeof(float));

            depth->NotifyAll();
            frameData->NotifyAll();
            cameraParams->NotifyAll();

            lastPresentedCloudId = cloudId;
        } else if (event.type == ENET_EVENT_TYPE_DISCONNECT) {
            printf("Disconnected from depth server!!\n");
            break;
        } else if (event.type == ENET_EVENT_TYPE_NONE) {
            break;
        }
    }
}
