//
// Created by Kyle Smith on 2026-06-18.
//

#include "pointmapper/pipeline/nodes/NetworkSendNode.h"

pointmapper::pipeline::NetworkSendNode::NetworkSendNode() {
    ENetAddress address = {0};
    address.port = 4567;
    enet_address_set_host_ip_new(&address, "0.0.0.0");

    server = enet_host_create(&address,
        32, // 32 clients
        2, // 2 channels per client. Channel 0 is point data and channel 1 is control signals
        0, // no incoming bandwidth limit
        0 // no outgoing bandwidth limit
    );

    ProcessLazily = false;

    cloud = CreateInput<CPUPointCloud>();
    currentId = 1;
}

pointmapper::pipeline::NetworkSendNode::~NetworkSendNode() {
    enet_host_destroy(server);
    free(compressionBuffer);
}

void pointmapper::pipeline::NetworkSendNode::Hydrate() {
    compressionBufferSize = (*cloud)->maximumPointCount * sizeof(PointXYZRGB);
    compressionBuffer = malloc(compressionBufferSize * 2);
    field = zfp_field_2d(nullptr, zfp_type_float, 0, 3);
    zfp_field_set_stride_2d(field, 4, 1);

    outstream = stream_open(compressionBuffer, compressionBufferSize);

    zfp = zfp_stream_open(outstream);
    //zfp_stream_set_rate(zfp, 6, zfp_type_float, 2, false);
    zfp_stream_set_accuracy(zfp, 0.01); // 1cm inaccuracy
    //zfp_stream_set_mode(zfp, zfp_mode_reversible);
    //zfp_stream_set_reversible(zfp);
}

void pointmapper::pipeline::NetworkSendNode::Process(PipelineBundle &) {
    ENetEvent event;
    enet_host_service(server, &event, 1);

    auto info = net::NetInfoPacket {
        .maxPoints = net::to_network_order((*cloud)->maximumPointCount)
    };
    ENetPacket *packet = nullptr;
    switch (event.type) {
        case ENET_EVENT_TYPE_CONNECT:
            printf("Client connected from :%u\n", event.peer->address.port);
            // send the info packet immediately

            printf("max point count: %u\n", (*cloud)->maximumPointCount);
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

    if (cloud->HasNewData()) {
        auto& points = *(*cloud).operator->();
        //printf("Sending %lu points\n", points.points.size());
        if (points.points.size() < 100) return;

        zfp_field_set_pointer(field, points.points.data());
        zfp_field_set_size_2d(field, points.points.size(), 3);

        zfp_stream_rewind(zfp);

        zfp_write_header(zfp, field, ZFP_HEADER_FULL);
        auto osize = zfp_compress(zfp, field);

        //printf("Compressed %lu byes to %lu bytes\n", points.points.size() * 12, osize);

        auto packet = enet_packet_create(&points, osize + sizeof(net::NetHeader), ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT);
        auto header = net::NetHeader {
            .magic = {'C', 'L', 'D'},
            .kind = 'C', // c == compressed points
            .cloud_id = net::to_network_order(currentId)
        };
        memcpy(packet->data, &header, sizeof(header));
        memcpy(packet->data + sizeof(net::NetHeader),  compressionBuffer, osize);

        enet_host_broadcast(server, 0, packet);
        enet_host_flush(server);
        currentId++;
    }
}
