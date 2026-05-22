//
// Created by Kyle Smith on 2026-05-21.
//

#include "RealsenseDevice.h"

#include "RealsenseFrame.h"


RealsenseDevice::RealsenseDevice() {
    pipeline = rs2::pipeline();
    rs2::config cfg;
    cfg.enable_stream(RS2_STREAM_DEPTH, -1, 640, 480, RS2_FORMAT_Z16, 30);
    cfg.enable_stream(RS2_STREAM_COLOR, -1, 640, 480, RS2_FORMAT_RGBA8, 30);
    pipeline.start(cfg);
    static_cast<void>(pipeline.wait_for_frames());

    frame = std::make_shared<RealsenseFrame>();
}

CameraParams RealsenseDevice::GetCameraParameters() {
    auto profile = pipeline.get_active_profile().get_stream(RS2_STREAM_DEPTH).as<rs2::video_stream_profile>();
    auto intrinsics = profile.get_intrinsics();

    return {
        .fx = intrinsics.fx,
        .fy = intrinsics.fy,
        .cx = intrinsics.ppx,
        .cy = intrinsics.ppy,
        .width = intrinsics.width,
        .height = intrinsics.height,
    };
}

std::string RealsenseDevice::GetName() {
    auto x = pipeline.get_active_profile().get_device();
    return x.get_type();
}

std::shared_ptr<Frame> RealsenseDevice::GetNextFrame() {
    auto didGetFrame = pipeline.poll_for_frames(&frame->frames);
    if (didGetFrame) {
        frame->ConvertDepth();
        return std::static_pointer_cast<Frame>(frame);
    } else {
        return nullptr;
    }

}
