struct Point { // layout identical to PointXYZRGB
    pos: vec4<f32> // need to bitcase w to u32 then use unpack4x8unorm, color channels are packed
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

struct RegistrationData {
    m: mat4x4<f32>
}

@group(0)
@binding(0)
var<uniform> uniforms: RenderUniforms;

@group(1)
@binding(0)
var<storage, read> points: array<Point>;

@group(1)
@binding(1)
var<uniform> registration: RegistrationData;

var<immediate> m: ModelData;

@vertex
fn vs(i: Vertex) -> VertexOut {
    let idx = i.instance;
    let pt = points[idx];

    var result: VertexOut;

    let rotationMatrix = transpose(mat3x3<f32>(
        uniforms.viewMatrix[0].xyz,
        uniforms.viewMatrix[1].xyz,
        uniforms.viewMatrix[2].xyz
    ));

    let billboard = vec4(rotationMatrix * i.position.xzy, 1);

    result.pos = uniforms.projectionViewMatrix * m.m * ((billboard * 0.002) + (registration.m * vec4f(pt.pos.xyz, 1)));
    result.color = unpack4x8unorm(bitcast<u32>(pt.pos.w));

    return result;
}

@fragment
fn fs(i: VertexOut) -> @location(0) vec4f {
    return i.color;
}