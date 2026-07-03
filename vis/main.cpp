//
// Created by Kyle Smith on 2026-06-19.
//


#include "Engine.h"
#include "TestActor.h"
#include "../lib/kinect2/Kinect2Device.h"
#include "../lib/pipeline/PointmapperPipeline.h"
#include "../lib/pipeline/nodes/CpuToGpuCopyNode.h"
#include "../lib/pipeline/nodes/DepthCameraNode.h"
#include "../lib/pipeline/nodes/NetworkReceiveNode.h"
#include "../lib/pipeline/nodes/RemoveBackgroundNode.h"
#include "../lib/pipeline/nodes/RemoveBlobsNode.h"
#include "../lib/rendering/pipelines.h"

int main() {
    auto engine = new glengine::Engine("Point Visualizer", int2(1280, 720));
    engine->SetAllowNonFocusedPawnInput(true);
    auto renderer = engine->GetRenderer();
    addPointmapperPipelines(renderer);

    auto pipeline = new pointmapper::pipeline::PointmapperPipeline(renderer->GetDevice(), wgpuDeviceGetQueue(renderer->GetDevice()));
    auto input = pipeline->CreateRoot<pointmapper::pipeline::NetworkReceiveNode>("127.0.0.1", 4567);

    auto copy = pipeline->CreateNode<pointmapper::pipeline::CpuToGpuCopyNode>();
    copy->input->Connect(input->cloud);

    pipeline->Build();
    printf("Pipeline built!");

    auto actor = engine->SpawnActor<TestActor>();

    actor->SetNode(copy->output);

    while (true) {
        glfwPollEvents();
        pipeline->Process();
        engine->Update();
        engine->Render();
    }
}
