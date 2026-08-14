//
// Created by Kyle Smith on 2026-07-16.
//

#ifndef POINTMAPPER_DEPTHMAPRECEIVENODE_H
#define POINTMAPPER_DEPTHMAPRECEIVENODE_H

#include <string_view>
#include <vector>
#include <zfp.h>

#include "pointmapper/pipeline/enet.h"
#include "pointmapper/pipeline/nodes/Node.h"
#include "pointmapper/pipeline/types/CommonTypes.h"
#include "pointmapper/pipeline/net.h"

class DepthmapReceiveNode : public pointmapper::pipeline::Node {
public:
    DepthmapReceiveNode(std::string_view remoteAddress, uint16_t port);
    ~DepthmapReceiveNode() override;

    DepthmapReceiveNode(const DepthmapReceiveNode&) = delete;
    DepthmapReceiveNode& operator=(const DepthmapReceiveNode&) = delete;

    void Hydrate() override;
    void Process(pointmapper::pipeline::PipelineBundle&) override;

    std::shared_ptr<pointmapper::pipeline::Output<pointmapper::pipeline::GPUDepthMap>> depth;
    std::shared_ptr<pointmapper::pipeline::Output<CameraParams>> cameraParams;
    std::shared_ptr<pointmapper::pipeline::Output<pointmapper::pipeline::FrameData>> frameData;
private:
    ENetHost* client;
    ENetPeer* server;
    zfp_field* field;
    zfp_stream* zfp;
    std::vector<float> depthBuffer;
    uint32_t lastPresentedCloudId = 0;
};

#endif //POINTMAPPER_DEPTHMAPRECEIVENODE_H
