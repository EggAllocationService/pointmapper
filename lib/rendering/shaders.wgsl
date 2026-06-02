struct Point { // layout identical to PointXYZRGB
    pos: vec4<f32>,
    color: u32 // need to use unpack4x8unorm, color channels are packed
}

struct RenderUniforms {
    projectionViewMatrix: mat4x4<f32>,
    projectionMatrix: mat4x4<f32>,
    viewMatrix: mat4x4<f32>,
    lightCount: i32
}

struct Vertex {
    @location(0) position: vec4f,
    @builtin(instance_index) instance: u32
}
struct VertexIn {
    @location(0) pos: vec3f,
    @location(1) normal: vec3f,
    @location(2) uv: vec2f
}

struct VertexOut {
    @builtin(position) pos: vec4f,
    @location(0) color: vec4f
}

struct ModelData {
    m: mat4x4<f32>
}

@group(0)
@binding(0)
var<uniform> uniforms: RenderUniforms;

@group(1)
@binding(0)
var<storage, read> points: array<Point>;

var<immediate> m: ModelData;

@vertex
fn vs(i: Vertex) -> VertexOut {
    let idx = i.instance;
    let pt = points[idx];

    var result: VertexOut;

    result.pos = uniforms.projectionViewMatrix * m.m * ((i.position * 0.003) + pt.pos);
    result.color = unpack4x8unorm(pt.color);

    return result;
}

@fragment
fn fs(i: VertexOut) -> @location(0) vec4f {
    return i.color;
}