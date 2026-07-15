//
// Created by Kyle Smith on 2026-07-09.
//

#include "DepthmapSendNode.h"

#include "GpuToCpuCopyNode.h"
#include "../PointmapperPipeline.h"
#include "../net.h"

DepthmapSendNode::DepthmapSendNode(int port) {
    depth = CreateInput<pointmapper::pipeline::GPUDepthMap>();
    ENetAddress address = {0};
    address.port = port;

    enet_address_set_host_ip_new(&address, "0.0.0.0");

    server = enet_host_create(&address,
        32, // 32 clients
        3, // 2 channels per client. Channel 0 is point data and channel 1 is control signals
        0, // no incoming bandwidth limit
        0 // no outgoing bandwidth limit
    );

    ProcessLazily = false;
    currentId = 0;
}

void DepthmapSendNode::Hydrate() {
    const auto& d = *(*depth).operator->();
    dataSize = d.width * d.height * sizeof(float);
    compressionBuffer = malloc(dataSize);
    readBuffer = pointmapper::pipeline::PIPELINE->CreateBuffer(dataSize, WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead);

    field = zfp_field_2d(nullptr, zfp_type_float, d.width, d.height);
    outstream = stream_open(compressionBuffer, dataSize);

    zfp = zfp_stream_open(outstream);
    zfp_stream_set_accuracy(zfp, 0.01);
}

void DepthmapSendNode::Process(pointmapper::pipeline::PipelineBundle &bundle) {
    ENetEvent event;
    enet_host_service(server, &event, 1);

    auto info = pointmapper::pipeline::net::DepthMapInfoPacket {
        .width = pointmapper::pipeline::net::to_network_order((*depth)->width),
        .height = pointmapper::pipeline::net::to_network_order((*depth)->height),
        .cameraParams = pointmapper::pipeline::net::to_network_order(**cameraParams)
    };
    ENetPacket *packet = nullptr;
    switch (event.type) {
        case ENET_EVENT_TYPE_CONNECT:
            printf("[Depth] Client connected from :%u\n", event.peer->address.port);

            packet = enet_packet_create(&info, sizeof(info), ENET_PACKET_FLAG_RELIABLE);
            enet_peer_send(event.peer, 1, packet);
            enet_host_flush(server);
            break;
        case ENET_EVENT_TYPE_DISCONNECT:
            printf("Client disconnected\n");
            break;
        case ENET_EVENT_TYPE_RECEIVE:
            printf("Client sent packet for some reason? Disconnecting.\n");
            enet_peer_disconnect(event.peer, 0);
            break;
        case ENET_EVENT_TYPE_NONE:
        case ENET_EVENT_TYPE_DISCONNECT_TIMEOUT:
            // don't do anything
            break;
    }

    if (depth->HasNewData()) {
        bundle.EndComputePass();
        wgpuCommandEncoderCopyBufferToBuffer(bundle.cmd, (*depth)->buffer, 0, readBuffer, 0, dataSize);

        bundle.Flush();

        pointmapper::pipeline::GpuToCpuCopyNode::ReadbackBuffer(readBuffer, dataSize);

        auto frame = wgpuBufferGetMappedRange(readBuffer, 0, dataSize);
        zfp_field_set_pointer(field, frame);
        zfp_stream_rewind(zfp);
        zfp_write_header(zfp, field, ZFP_HEADER_FULL);
        auto osize = zfp_compress(zfp, field);
        printf("Compressed %lu byes to %lu bytes\n", dataSize, osize);

        auto packetSize = sizeof(pointmapper::pipeline::net::NetHeader) + sizeof(pointmapper::pipeline::FrameData) + osize;

        auto packet = enet_packet_create(nullptr, packetSize, ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT);
        *reinterpret_cast<pointmapper::pipeline::net::NetHeader*>(packet->data) = {
            .magic = {'C', 'L', 'D'},
            .kind = 'D', // 'D' = depth map
            .cloud_id = currentId
        };

        *reinterpret_cast<pointmapper::pipeline::FrameData*>(packet->data + sizeof(pointmapper::pipeline::net::NetHeader)) = **frameData;

        memcpy(packet->data + sizeof(pointmapper::pipeline::net::NetHeader) + sizeof(pointmapper::pipeline::FrameData), compressionBuffer, osize);

        enet_host_broadcast(server, 0, packet);

        wgpuBufferUnmap(readBuffer);
        currentId++;
    }
}
