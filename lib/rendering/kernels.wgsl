struct PipelineInfo {
    fx: f32,
    fy: f32,
    cx: f32,
    cy: f32,
    depth_width: u32,
    depth_height: u32,
    color_width: u32,
    color_height: u32,
    depth_tolerance: f32
}

struct OutputInfo {
    vertexCount: u32,
    instanceCount: atomic<u32>,
    firstVertex: u32,
    firstInstance: u32
}

struct Point { // layout identical to PointXYZRGB
    pos: vec4<f32> // color is stored in w, packed 4 bytes. need to bitcast to u32 and unpack to get them
}

struct MaskPushConstants {
    scale: f32
}

struct CloudPushConstants {
    scalar: vec4<f32>,
    depth_scale: f32
}

// group 0 - uniforms
// group 1 - depth i/o
// group 2 - masking state

@group(0)
@binding(0)
var<uniform> info: PipelineInfo;

@group(0)
@binding(1)
var<storage, read_write> outInfo: OutputInfo;

@group(1)
@binding(0)
var<storage, read_write> depth: array<f32>;

@group(1)
@binding(1)
var<storage, read_write> output: array<Point>;

@group(2)
@binding(0)
var<storage, read_write> max_depth: array<f32>;
@group(2)
@binding(1)
var<storage, read_write> prev_depth: array<f32>;

@compute @workgroup_size(8, 8)
fn mask(@builtin(global_invocation_id) pos: vec3<u32>) {
    if (pos.x > info.depth_width || pos.y > info.depth_height) {
        return;
    }

    let idx = (pos.y * info.depth_width) + pos.x;
    var d = depth[idx]; ///* c.scale;
    if d < 0 {
        d = 0;
    }
    if (prev_depth[idx] == 0) {
        depth[idx] = d;
        prev_depth[idx] = d;
    } else if (d == 0) {
        prev_depth[idx] = 0;
        depth[idx] = 0;
    } else {
        var filtered = (prev_depth[idx] * 0.7) + (d * 0.3);

        prev_depth[idx] = filtered;

        depth[idx] = filtered;
        d = filtered;
    }

   max_depth[idx] = max(max_depth[idx], d);

    let delta = abs(max_depth[idx] - d);

    if delta < info.depth_tolerance {
        depth[idx] = 0;
        max_depth[idx] = d;
    }
}


@group(2)
@binding(0)
var texSampler: sampler;
@group(2)
@binding(1)
var colorTex: texture_2d<f32>;

var<immediate> cc: CloudPushConstants;
@compute @workgroup_size(8, 8)
fn create_cloud(@builtin(global_invocation_id) pos: vec3<u32>) {
    if (pos.x > info.depth_width || pos.y > info.depth_height) {
        return;
    }

    let uv = vec2f(pos.xy) / vec2f(f32(info.depth_width), f32(info.depth_height));
    let idx = (pos.y * info.depth_width) + pos.x;
    let fpos = vec3f(pos);

    let d = depth[idx] * cc.depth_scale;
    let x = (fpos.x - info.cx) * (d/info.fx);
    let y = (fpos.y - info.cy) * (d/info.fy);
    let z = d;

    if d != 0 && d < 4 {
        let oIdx = atomicAdd(&outInfo.instanceCount, 1);
        let color = textureSampleLevel(colorTex, texSampler, uv, 0.0);
        output[oIdx].pos = vec4f(vec3f(x, y, z) * cc.scalar.xyz, bitcast<f32>(pack4x8unorm(color)));
    }
}
struct WorkgroupInfo {
    edges: atomic<u32>,
    min: atomic<u32>,
    max: atomic<u32>
}
var<workgroup> wg: WorkgroupInfo;

@compute @workgroup_size(8, 8)
fn remove_blobs(@builtin(global_invocation_id) pos: vec3<u32>, @builtin(local_invocation_id) wgPos: vec3<u32>) {
    if (wgPos.x == 0 && wgPos.y == 0) {
        atomicStore(&wg.max, 1);
    }

    let idx = (pos.y * info.depth_width) + pos.x;

    if (wgPos.x == 0 || wgPos.y == 0 || wgPos.x == 7 || wgPos.y == 7) {
        if (depth[idx] != 0) {
            atomicAdd(&wg.edges, 1);
            atomicMin(&wg.min, u32(depth[idx] * 1000));
            atomicMax(&wg.max, u32(depth[idx] * 1000));
        }
    }

    workgroupBarrier();
    if wg.edges < 8 {
        depth[idx] = 0;
    } else if wg.edges < 16 && depth[idx] != 0 {
        let min = f32(wg.min) / 1000.0;
        let max = f32(wg.max) / 1000.0;

        let norm = (depth[idx] - min) / (max - min);

        // if there's less than 50% valid pixels along the edge, erase the whole workgroup
        if (norm < 0.7 && norm > 0.3) {
            depth[idx] = max;
        }
    }

}