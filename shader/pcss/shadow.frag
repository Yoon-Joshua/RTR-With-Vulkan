#version 460

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec4 inPosition;

vec4 pack (float depth) {
    // 
    const vec4 bitShift = vec4(1.0, 256.0, 256.0 * 256.0, 256.0 * 256.0 * 256.0);
    const vec4 bitMask = vec4(1.0/256.0, 1.0/256.0, 1.0/256.0, 0.0);
    // gl_FragCoord:
    vec4 rgbaDepth = fract(depth * bitShift); //
    rgbaDepth -= rgbaDepth.gbaa * bitMask; // Cut off the value which do not fit in 8 bits
    return rgbaDepth;
}

void main(){
    outColor = vec4(inPosition.zzz / inPosition.w, 1.0);
}