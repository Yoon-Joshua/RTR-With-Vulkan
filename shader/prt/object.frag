#version 460

layout(location = 0) in vec2 inTexCoord;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec4 outColor;

vec3 toneMapping(vec3 color){
    vec3 result;

    for (int i=0; i<3; ++i) {
        if (color[i] <= 0.0031308)
            result[i] = 12.92 * color[i];
        else
            result[i] = (1.0 + 0.055) * pow(color[i], 1.0/2.4) - 0.055;
    }

    return result;
}

void main(){
    //outColor = vec4(toneMapping(inColor), 1.0);
    outColor = vec4(inColor, 1.0);
}