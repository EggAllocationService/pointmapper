//
// Created by Kyle Smith on 2026-06-19.
//

#ifndef POINTMAPPER_NETWORKRECIEVENODE_H
#define POINTMAPPER_NETWORKRECIEVENODE_H
#include <string_view>
#include <enet/enet.h>

#include "Node.h"
#include "../types/CommonTypes.h"

namespace pointmapper::pipeline {
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
    private:
        ENetHost *client;
        ENetPeer *server;
    };
}

#endif //POINTMAPPER_NETWORKRECIEVENODE_H
