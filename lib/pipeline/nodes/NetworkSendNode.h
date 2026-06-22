//
// Created by Kyle Smith on 2026-06-18.
//

#ifndef POINTMAPPER_NETWORKSENDNODE_H
#define POINTMAPPER_NETWORKSENDNODE_H
#include <enet/enet.h>

#include "Node.h"
#include "../types/CommonTypes.h"
#include "../net.h"


namespace pointmapper::pipeline {
    class NetworkSendNode : public Node {
    public:
        NetworkSendNode();
        ~NetworkSendNode() override;

        // Non-copyable
        NetworkSendNode(const NetworkSendNode& other) = delete;
        NetworkSendNode& operator=(const NetworkSendNode& other) = delete;

        void Hydrate() override;

        void Process(PipelineBundle &) override;


        std::shared_ptr<Input<CPUPointCloud>> cloud;

    private:
        ENetHost* server;
        uint32_t currentId;
    };
}

#endif //POINTMAPPER_NETWORKSENDNODE_H
