//
// Created by Kyle Smith on 2026-06-11.
//

#ifndef POINTMAPPER_DEPTHCAMERANODE_H
#define POINTMAPPER_DEPTHCAMERANODE_H

#include "Node.h"
#include "../../DepthDevice.h"
#include "../types/CommonTypes.h"

#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

namespace pointmapper::pipeline {
    class DepthCameraNode : public Node {
    public:
        explicit DepthCameraNode(DepthDevice* device);
        ~DepthCameraNode() override;

        void Hydrate() override;
        void Process(PipelineBundle&) override;

        std::shared_ptr<Output<GPUDepthMap>> depth;
        std::shared_ptr<Output<CameraParams>> params;
        std::shared_ptr<Output<GPUColorTexture>> color;
        std::shared_ptr<Output<FrameData>> frameData;

    private:
        DepthDevice* device;
        CameraParams cameraParams{};

        struct PendingFrame {
            std::vector<float> depth;
            std::vector<uint32_t> color;
            float depthUnits = 1.0f;
            float axisScale[4] = {1.0f, 0.0f, 0.0f, 0.0f};
        };

        std::mutex pendingMutex;
        std::condition_variable pendingCV;
        PendingFrame pendingFrame;
        bool hasPendingFrame = false;
        bool stopThread = false;

        std::thread captureThread;

        void CaptureThread();
    };
}

#endif //POINTMAPPER_DEPTHCAMERANODE_H
