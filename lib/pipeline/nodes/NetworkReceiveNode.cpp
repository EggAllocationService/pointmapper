//
// Created by Kyle Smith on 2026-06-19.
//

#include "NetworkReceiveNode.h"

#include <cassert>

#include "../net.h"

pointmapper::pipeline::NetworkReceiveNode::NetworkReceiveNode(std::string_view remoteAddress) {
    std::string remote(remoteAddress);
    ENetAddress address;
    enet_address_set_host(&address, remote.c_str());

    client = enet_host_create(
      nullptr,
      1,
      2,
      0,
      0
    );
    assert(client);

    server = enet_host_connect(client, &address, 2, 0);

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
    enet_host_service(client, &event, 0);
    if (event.type == ENET_EVENT_TYPE_RECEIVE && event.channelID == 0) {
        printf("Receiving point data\n");
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
            printf("Recieved pointcloud exceeds maximum point count! (%lu/%d)\n", pointCount, (*cloud)->maximumPointCount);
            return;
        }

        (*cloud)->points.resize(pointCount);
        memcpy((*cloud)->points.data(), data, dataLen);

        cloud->NotifyAll();
    } else if (event.type == ENET_EVENT_TYPE_DISCONNECT) {
        printf("Disconnected from server!!\n");
    }
}

pointmapper::pipeline::NetworkReceiveNode::~NetworkReceiveNode() {
    enet_host_destroy(client);
}
