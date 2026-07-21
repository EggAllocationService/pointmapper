//
// Created by Kyle Smith on 2026-06-18.
//

#ifndef POINTMAPPER_NETWORKSENDNODE_H
#define POINTMAPPER_NETWORKSENDNODE_H
#include "../enet.h"

#include "Node.h"
#include "../types/CommonTypes.h"
#include "../net.h"
#include <zfp.h>


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
        void* compressionBuffer;
        size_t compressionBufferSize;
        zfp_field* field;
        zfp_stream* zfp;
        bitstream* outstream;
        uint32_t currentId;
    };
}

#endif //POINTMAPPER_NETWORKSENDNODE_H
