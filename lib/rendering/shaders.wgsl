struct Point { // layout identical to PointXYZRGB
    pos: vec4<f32>,
    color: u32 // need to use unpack4x8unorm, color channels are packed
}

