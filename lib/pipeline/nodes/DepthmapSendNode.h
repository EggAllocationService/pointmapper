//
// Created by Kyle Smith on 2026-07-09.
//

#ifndef POINTMAPPER_DEPTHMAPSENDNODE_H
#define POINTMAPPER_DEPTHMAPSENDNODE_H
#include <zfp.h>

#include "Node.h"
#include "../types/CommonTypes.h"
#include "../enet.h"



class DepthmapSendNode : public pointmapper::pipeline::Node {
public:
    DepthmapSendNode(int port);

    void Hydrate() override;

    void Process(pointmapper::pipeline::PipelineBundle &) override;

    std::shared_ptr<pointmapper::pipeline::Input<pointmapper::pipeline::GPUDepthMap>> depth;
    std::shared_ptr<pointmapper::pipeline::Input<CameraParams>> cameraParams;
    std::shared_ptr<pointmapper::pipeline::Input<pointmapper::pipeline::FrameData>> frameData;
private:
    WGPUBuffer readBuffer;
    ENetHost* server;
    void* compressionBuffer;
    size_t dataSize;
    zfp_field* field;
    zfp_stream* zfp;
    bitstream* outstream;
    uint32_t currentId;
};


#endif //POINTMAPPER_DEPTHMAPSENDNODE_H
