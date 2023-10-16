#version 460

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in mat3 transport;

layout(location = 0) out vec4 outColor;

layout(binding = 1, set = 0) uniform sampler2D texSampler;

layout(binding = 2, set = 0) uniform LightPRT {
	mat3 r;
	mat3 g;
	mat3 b;
} lightPrt;

void main(){
	
	vec4 intensity;

	intensity.a = 1.0;

	intensity.r = lightPrt.r[0][0] * transport[0][0] + lightPrt.r[0][1] * transport[0][1] + lightPrt.r[0][2] * transport[0][2]
				+ lightPrt.r[1][0] * transport[1][0] + lightPrt.r[1][1] * transport[1][1] + lightPrt.r[1][2] * transport[1][2]
				+ lightPrt.r[2][0] * transport[2][0] + lightPrt.r[2][1] * transport[2][1] + lightPrt.r[2][2] * transport[2][2];

    intensity.g = lightPrt.g[0][0] * transport[0][0] + lightPrt.g[0][1] * transport[0][1] + lightPrt.g[0][2] * transport[0][2]
				+ lightPrt.g[1][0] * transport[1][0] + lightPrt.g[1][1] * transport[1][1] + lightPrt.g[1][2] * transport[1][2]
				+ lightPrt.g[2][0] * transport[2][0] + lightPrt.g[2][1] * transport[2][1] + lightPrt.g[2][2] * transport[2][2];

	intensity.b = lightPrt.b[0][0] * transport[0][0] + lightPrt.b[0][1] * transport[0][1] + lightPrt.b[0][2] * transport[0][2]
				+ lightPrt.b[1][0] * transport[1][0] + lightPrt.b[1][1] * transport[1][1] + lightPrt.b[1][2] * transport[1][2]
				+ lightPrt.b[2][0] * transport[2][0] + lightPrt.b[2][1] * transport[2][1] + lightPrt.b[2][2] * transport[2][2];

    outColor = intensity;
	//outColor = texture(texSampler, inTexCoord) * vec4(intensity, 1.0);
}