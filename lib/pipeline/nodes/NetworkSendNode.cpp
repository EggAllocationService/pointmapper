//
// Created by Kyle Smith on 2026-06-18.
//

#include "NetworkSendNode.h"

pointmapper::pipeline::NetworkSendNode::NetworkSendNode() {
    ENetAddress address = {
        .host = ENET_HOST_ANY,
        .port = 4567
    };

    server = enet_host_create(&address,
        32, // 32 clients
        2, // 2 channels per client. Channel 0 is point data and channel 1 is control signals
        0, // no incoming bandwidth limit
        0 // no outgoing bandwidth limit
    );
    ProcessLazily = false;

    cloud = CreateInput<CPUPointCloud>();
}

pointmapper::pipeline::NetworkSendNode::~NetworkSendNode() {
    enet_host_destroy(server);
}

void pointmapper::pipeline::NetworkSendNode::Hydrate() {
}

void pointmapper::pipeline::NetworkSendNode::Process(PipelineBundle &) {
    ENetEvent event;
    enet_host_service(server, &event, 0);

    auto info = net::NetInfoPacket {
        .maxPoints = net::to_network_order((*cloud)->maximumPointCount)
    };
    ENetPacket *packet = nullptr;
    switch (event.type) {
        case ENET_EVENT_TYPE_CONNECT:
            printf("Client connected from %x:%u\n", event.peer->address.host, event.peer->address.port);
            // send the info packet immediately
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
            // don't do anything
            break;
    }

    if (cloud->HasNewData()) {
        auto& points = *(*cloud).operator->();

        auto packet = enet_packet_create(nullptr, sizeof(net::NetHeader) + (sizeof(PointXYZRGB) * points.points.size()), 0);

        auto header = net::NetHeader{
            .magic = {'C', 'L', 'D'},
            .kind = 'P' // P for points
        };
        memcpy(packet->data, &header, sizeof(header));

        // copy points
        memcpy(packet->data + sizeof(header), points.points.data(), points.points.size() * sizeof(PointXYZRGB));

        enet_host_broadcast(server, 0, packet);

        enet_host_flush(server);
    }
}
