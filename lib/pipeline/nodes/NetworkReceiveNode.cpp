//
// Created by Kyle Smith on 2026-06-19.
//

#include "NetworkReceiveNode.h"

#include <cassert>

#include "../net.h"

pointmapper::pipeline::NetworkReceiveNode::NetworkReceiveNode(std::string_view remoteAddress, uint16_t port) {
    std::string remote(remoteAddress);
    ENetAddress address;
    address.port = port;
    enet_address_set_host_ip(&address, remote.c_str());

    client = enet_host_create(
      nullptr,
      1,
      2,
      0,
      0
    );
    assert(client);
    enet_host_compress_with_range_coder(client);

    server = enet_host_connect(client, &address, 2, 0);


    cloud = CreateOutput<CPUPointCloud>();

    ProcessLazily = false;
}

void pointmapper::pipeline::NetworkReceiveNode::Hydrate() {
    ENetEvent event;
    while (true) {
        enet_host_service(client, &event, 10);
        if (event.type == ENET_EVENT_TYPE_CONNECT) {
            printf("Connected to server\n");
            continue;
        }

        if (event.type == ENET_EVENT_TYPE_RECEIVE) {
            if (event.channelID != 1 || event.packet->dataLength != sizeof(net::NetInfoPacket)) {
                printf("WARN: receive packet during init which wasn't on channel 1!\n");
                continue;
            }
            auto info = reinterpret_cast<net::NetInfoPacket*>(event.packet->data);
            auto length = net::to_network_order(info->maxPoints);
            printf("Connection established! Maximum point size is %u\n", length);

            (*cloud)->maximumPointCount = length;
            (*cloud)->points.reserve(length);

            cloud->MarkReady();
            break;
        }
    }
}

void pointmapper::pipeline::NetworkReceiveNode::Process(PipelineBundle &) {
    ENetEvent event;
    enet_host_service(client, &event, 1);
    if (event.type == ENET_EVENT_TYPE_RECEIVE && event.channelID == 0) {
        auto packet = event.packet;
        auto header = *reinterpret_cast<net::NetHeader*>(packet->data);
        // verify header
        if (header.kind != 'P') {
            printf("Malformed packet!\n");
            return;
        }

        auto data = packet->data + sizeof(net::NetHeader);
        auto dataLen = packet->dataLength - sizeof(net::NetHeader);
        if (dataLen % sizeof(PointXYZRGB) != 0) {
            printf("Packet does not contain an integral amount of points!\n");
            return;
        }

        auto pointCount = dataLen / sizeof(PointXYZRGB);
        if (pointCount > (*cloud)->maximumPointCount) {
            printf("Received pointcloud exceeds maximum point count! (%lu/%d)\n", pointCount, (*cloud)->maximumPointCount);
            return;
        }

        (*cloud)->points.resize(pointCount);
        memcpy((*cloud)->points.data(), data, dataLen);

        enet_packet_destroy(packet);
        cloud->NotifyAll();
    } else if (event.type == ENET_EVENT_TYPE_DISCONNECT) {
        printf("Disconnected from server!!\n");
    }

    enet_host_service(client, &event, 0);
    while (event.type != ENET_EVENT_TYPE_NONE) {
        enet_host_service(client, &event, 0);
    }
}

pointmapper::pipeline::NetworkReceiveNode::~NetworkReceiveNode() {
    enet_host_destroy(client);
}
