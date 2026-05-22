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
    validPoints: atomic<u32>
}

struct Point { // layout identical to PointXYZRGB
    pos: vec4<f32>,
    color: u32 // need to use unpack4x8unorm, color channels are packed
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

@group(3)
@binding(0)
var texSampler: sampler;
@group(3)
@binding(1)
var colorTex: texture_2d<f32>;

@compute @workgroup_size(1)
fn mask(@builtin(global_invocation_id) pos: vec3<u32>) {
    let idx = (pos.y * info.depth_width) + pos.x;
    var d = depth[idx];
    if d > 4 {
        d = 1000;
    }

    var filtered: f32 = 0;

    if abs(d - prev_depth[idx]) > 0.5 {
        filtered = 0;
    } else {
        filtered = (d + prev_depth[idx]) / 2.0;
    }

    prev_depth[idx] = d;

    max_depth[idx] = max(max_depth[idx], filtered);

    let delta = abs(max_depth[idx] - filtered);

    if delta < info.depth_tolerance {
        depth[idx] = 0;
        max_depth[idx] = filtered;
    }
}

@compute @workgroup_size(1)
fn create_cloud(@builtin(global_invocation_id) pos: vec3<u32>) {

    let uv = vec2f(pos.xy) / vec2f(f32(info.depth_width), f32(info.depth_height));
    let idx = (pos.y * info.depth_width) + pos.x;
    let fpos = vec3f(pos);

    let d = depth[idx];
    let x = (fpos.x - info.cx) * (d/info.fx);
    let y = (fpos.y - info.cy) * (d/info.fy);
    let z = d;

    let oIdx = atomicAdd(&outInfo.validPoints, 1);

    output[oIdx].pos = vec4f(x, y, z, 1);
    if d == 0 || d > 4 {
        output[oIdx].color = 0;
    } else {
        let color = textureSampleLevel(colorTex, texSampler, uv, 0.0);
        output[oIdx].color = pack4x8unorm(color.bgra);
    }
}