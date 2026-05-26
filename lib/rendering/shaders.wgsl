struct Point { // layout identical to PointXYZRGB
    pos: vec4<f32>,
    color: u32 // need to use unpack4x8unorm, color channels are packed
}

struct Uniforms {
    worldViewMatrix: mat4x4<f32>
}

struct Vertex {
    @location(0) position: vec4f,
    @builtin(instance_index) instance: u32
}

struct VertexOut {
    @builtin(position) pos: vec4f,
    @location(0) color: vec4f
}

@group(0)
@binding(0)
var<uniform> uniforms: Uniforms;

@group(1)
@binding(0)
var<storage> points: array<Point>;

@vertex
fn vs(i: Vertex) -> VertexOut {
    let idx = i.instance;
    let pt = points[idx];

    var result: VertexOut;

    result.pos = (uniforms.worldViewMatrix * i.position) + pt.pos;
    result.color = unpack4x8unorm(pt.color);

    return result;
}

@fragment
fn fs(i: VertexOut) -> @location(0) vec4f {
    return vec4f(1, 1, 1, 1);
}