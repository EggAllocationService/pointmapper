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
                continue;
            }

            auto data = packet->data + sizeof(net::NetHeader);
            auto dataLen = packet->dataLength - sizeof(net::NetHeader);
            if (dataLen % sizeof(PointXYZRGB) != 0) {
                printf("Packet does not contain an integral amount of points!\n");
                enet_packet_destroy(packet);
                continue;
            }

            auto pointCount = dataLen / sizeof(PointXYZRGB);
            if (pointCount > (*cloud)->maximumPointCount) {
                printf("Received pointcloud exceeds maximum point count! (%lu/%d)\n", pointCount, (*cloud)->maximumPointCount);
                enet_packet_destroy(packet);
                continue;
            }

            // drop packets for clouds we've already presented
            if (header.cloud_id <= lastPresentedCloudId) {
                enet_packet_destroy(packet);
                continue;
            }

            // find or allocate a receive buffer for this cloud
            int slot = -1;
            for (int i = 0; i < NET_RECEIVE_BUFFER_COUNT; i++) {
                if (buffers[i].cloudId == header.cloud_id) {
                    slot = i;
                    break;
                }
            }
            if (slot == -1) {
                for (int i = 0; i < NET_RECEIVE_BUFFER_COUNT; i++) {
                    if (buffers[i].cloudId == 0) {
                        slot = i;
                        break;
                    }
                }
            }
            if (slot == -1) {
                // all buffers in use; evict the oldest cloud to make room
                int oldestSlot = 0;
                for (int i = 1; i < NET_RECEIVE_BUFFER_COUNT; i++) {
                    if (buffers[i].cloudId < buffers[oldestSlot].cloudId) {
                        oldestSlot = i;
                    }
                }
                if (header.cloud_id <= buffers[oldestSlot].cloudId) {
                    printf("No receive buffer available for cloud %u, dropping packet\n", header.cloud_id);
                    enet_packet_destroy(packet);
                    continue;
                }

                // present the evicted (oldest) cloud immediately
                auto& evicted = buffers[oldestSlot];
                (*cloud)->points = std::move(evicted.points);
                lastPresentedCloudId = evicted.cloudId;
                cloud->NotifyAll();

                // discard all older/stale buffered clouds
                for (int i = 0; i < NET_RECEIVE_BUFFER_COUNT; i++) {
                    if (buffers[i].cloudId != 0 && buffers[i].cloudId <= lastPresentedCloudId) {
                        buffers[i].cloudId = 0;
                        buffers[i].totalLength = 0;
                        buffers[i].received = 0;
                        buffers[i].points.clear();
                    }
                }
                slot = oldestSlot;
            }

            // initialize a fresh buffer for a new cloud id
            if (buffers[slot].cloudId == 0) {
                buffers[slot].cloudId = header.cloud_id;
                buffers[slot].totalLength = header.total_length;
                buffers[slot].points.clear();
                buffers[slot].points.reserve(header.total_length);
                buffers[slot].received = 0;
            }

            // consistency check against already initialized buffer
            if (buffers[slot].totalLength != header.total_length) {
                printf("Inconsistent total_length for cloud %u, dropping packet\n", header.cloud_id);
                enet_packet_destroy(packet);
                continue;
            }

            if (buffers[slot].received + pointCount > buffers[slot].totalLength) {
                printf("Received too many points for cloud %u, dropping packet\n", header.cloud_id);
                enet_packet_destroy(packet);
                continue;
            }

            buffers[slot].points.insert(buffers[slot].points.end(), reinterpret_cast<PointXYZRGB*>(data), reinterpret_cast<PointXYZRGB*>(data) + pointCount);
            buffers[slot].received += pointCount;

            enet_packet_destroy(packet);

            // find the highest-id completed cloud and present it if it's newer
            int completedSlot = -1;
            uint32_t completedId = lastPresentedCloudId;
            for (int i = 0; i < NET_RECEIVE_BUFFER_COUNT; i++) {
                if (buffers[i].cloudId != 0 &&
                    buffers[i].cloudId > completedId &&
                    buffers[i].received >= buffers[i].totalLength) {
                    completedId = buffers[i].cloudId;
                    completedSlot = i;
                }
            }
            if (completedSlot != -1) {
                auto& buf = buffers[completedSlot];
                (*cloud)->points = std::move(buf.points);
                lastPresentedCloudId = buf.cloudId;
                buf.cloudId = 0;
                buf.totalLength = 0;
                buf.received = 0;
                cloud->NotifyAll();

                // discard all older/stale buffered clouds
                for (int i = 0; i < NET_RECEIVE_BUFFER_COUNT; i++) {
                    if (buffers[i].cloudId != 0 && buffers[i].cloudId <= lastPresentedCloudId) {
                        buffers[i].cloudId = 0;
                        buffers[i].totalLength = 0;
                        buffers[i].received = 0;
                        buffers[i].points.clear();
                    }
                }
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
