//
// Created by Kyle Smith on 2026-06-19.
//

#include "NetworkReceiveNode.h"

#include <cassert>

#include "../net.h"

pointmapper::pipeline::NetworkReceiveNode::NetworkReceiveNode(std::string_view remoteAddress, uint16_t port) {
    std::string remote(remoteAddress);
    ENetAddress address = {0};
    address.port = port;
    address.host = in6addr_loopback;

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


    cloud = CreateOutput<CPUPointCloud>();

    ProcessLazily = false;
    readCount = 0;
}

void pointmapper::pipeline::NetworkReceiveNode::Hydrate() {
    ENetEvent event;
    enet_host_service(client, &event, 7000);
    if (event.type != ENET_EVENT_TYPE_CONNECT) {
        printf("Failed to connect to server (%u)\n", event.type);
        return;
    }

    while (true) {
        enet_host_service(client, &event, 10);

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
    int received = 0;
    while (true) {
        ENetEvent event;
        enet_host_service(client, &event, 0);
        if (event.type == ENET_EVENT_TYPE_RECEIVE && event.channelID == 0) {
            received++;
            auto packet = event.packet;
            auto header = *reinterpret_cast<net::NetHeader*>(packet->data);
            header.cloud_id = net::to_network_order(header.cloud_id);
            header.packet_length = net::to_network_order(header.packet_length);
            header.total_length = net::to_network_order(header.total_length);
            // verify header
            if (header.kind != 'P' || header.total_length > (*cloud)->maximumPointCount || header.packet_length > NET_MAX_PACKET_SIZE) {
                printf("Malformed packet!");
                enet_packet_destroy(packet);
                return;
            }

            if (header.cloud_id < receiveId) {
                // out of order packet, ignore
                enet_packet_destroy(packet);
                return;
            }
            //printf("Recv cloud=%u target=%u segment=%u total=%u\n", header.cloud_id, header.total_length, header.packet_length, readCount);
            if (header.cloud_id > this->receiveId) {
                // new packet series
                (*cloud)->points.resize(header.total_length);
                this->receiveId = header.cloud_id;
                readCount = 0;
            }

            auto data = packet->data + sizeof(net::NetHeader);
            auto dataLen = packet->dataLength - sizeof(net::NetHeader);
            if (dataLen % sizeof(PointXYZRGB) != 0) {
                printf("Packet does not contain an integral amount of points!\n");
                enet_packet_destroy(packet);
                return;
            }

            auto pointCount = dataLen / sizeof(PointXYZRGB);
            if (pointCount > (*cloud)->maximumPointCount) {
                printf("Received pointcloud exceeds maximum point count! (%lu/%d)\n", pointCount, (*cloud)->maximumPointCount);
                enet_packet_destroy(packet);
                return;
            }

            memcpy((*cloud)->points.data() + readCount, data, dataLen);
            readCount += header.packet_length;

            enet_packet_destroy(packet);
            if (readCount == header.total_length) {
                cloud->NotifyAll();
                break;
            }

        } else if (event.type == ENET_EVENT_TYPE_DISCONNECT) {
            printf("Disconnected from server!!\n");
        } else if (event.type == ENET_EVENT_TYPE_NONE) {
            break;
        }
    }

    if (received > 0) {
        //printf("Received %d packets\n", received);
    }
}

pointmapper::pipeline::NetworkReceiveNode::~NetworkReceiveNode() {
    enet_host_destroy(client);
}
