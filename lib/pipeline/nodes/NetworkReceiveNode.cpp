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

    cloud = CreateOutput<CPUPointCloud>();

    ProcessLazily = false;
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

    field = zfp_field_alloc();
    zfp = zfp_stream_open(nullptr);
    zfp_field_set_pointer(field, (*cloud)->points.data());
    zfp_field_set_stride_2d(field, 4, 1);
}

void pointmapper::pipeline::NetworkReceiveNode::Process(PipelineBundle &) {
    while (true) {
        ENetEvent event;
        enet_host_service(client, &event, 0);
        if (event.type == ENET_EVENT_TYPE_RECEIVE && event.channelID == 0) {
            auto packet = event.packet;
            auto header = *reinterpret_cast<net::NetHeader*>(packet->data);
            header.cloud_id = net::to_network_order(header.cloud_id);

            if (header.cloud_id <= lastPresentedCloudId) {
                enet_packet_destroy(packet);
                break;
            }

            auto instream = stream_open(packet->data + sizeof(header), packet->dataLength - sizeof(header));
            zfp_stream_set_bit_stream(zfp, instream);
            zfp_stream_rewind(zfp);
            zfp_read_header(zfp, field, ZFP_HEADER_FULL);
            size_t dimensions[4];
            zfp_field_size(field, dimensions);
            zfp_field_set_stride_2d(field, 4, 1);
            //printf("Received %lu points\n", dimensions[0]);
            (*cloud)->points.resize(dimensions[0]);
            zfp_decompress(zfp, field);

            stream_close(instream);
            enet_packet_destroy(packet);
            cloud->NotifyAll();
        } else if (event.type == ENET_EVENT_TYPE_DISCONNECT) {
            printf("Disconnected from server!!\n");
        } else if (event.type == ENET_EVENT_TYPE_NONE) {
            break;
        }
    }
}

pointmapper::pipeline::NetworkReceiveNode::~NetworkReceiveNode() {
    enet_host_destroy(client);
}
