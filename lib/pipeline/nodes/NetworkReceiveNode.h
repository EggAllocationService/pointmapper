//
// Created by Kyle Smith on 2026-06-19.
//

#ifndef POINTMAPPER_NETWORKRECIEVENODE_H
#define POINTMAPPER_NETWORKRECIEVENODE_H
#include <string_view>
#include "../enet.h"

#include "Node.h"
#include "../types/CommonTypes.h"
#include "../net.h"

namespace pointmapper::pipeline {
    namespace net {
        struct ReceiveBuffer {
            std::vector<PointXYZRGB> points;
            uint32_t cloudId = 0;
            uint32_t totalLength = 0;
            uint32_t received = 0;
        };
    }
    class NetworkReceiveNode : public Node {
    public:
        NetworkReceiveNode(std::string_view remoteAddress, uint16_t port);
        ~NetworkReceiveNode() override;

        // Non-copyable
        NetworkReceiveNode(const NetworkReceiveNode& other) = delete;
        NetworkReceiveNode& operator=(const NetworkReceiveNode& other) = delete;

        void Hydrate() override;

        void Process(PipelineBundle &) override;

        std::shared_ptr<Output<CPUPointCloud>> cloud;
        net::ReceiveBuffer buffers[NET_RECEIVE_BUFFER_COUNT];
    private:
        ENetHost *client;
        ENetPeer *server;
        uint32_t lastPresentedCloudId = 0;
    };
}

#endif //POINTMAPPER_NETWORKRECIEVENODE_H
