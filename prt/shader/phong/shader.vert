#version 460

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 transport0;
layout(location = 3) in vec3 transport1;
layout(location = 4) in vec3 transport2;

layout(location = 0) out vec3 outPosition;
layout(location = 1) out vec2 outTexCoord;
layout(location = 2) out vec3 outColor;

layout(binding = 0, set = 0) uniform UniformObject{
	mat4 model;
	mat4 view;
	mat4 proj;
} ubo;

layout(binding = 1, set = 0) uniform LightPRT {
	mat3 r;
	mat3 g;
	mat3 b;
} lightPrt;

void main(){
	gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
	outPosition = (ubo.model * vec4(inPosition, 1.0)).xyz;
	outTexCoord = inTexCoord;

	outColor.r = lightPrt.r[0][0] * transport0[0] + lightPrt.r[0][1] * transport0[1] + lightPrt.r[0][2] * transport0[2]
				+ lightPrt.r[1][0] * transport1[0] + lightPrt.r[1][1] * transport1[1] + lightPrt.r[1][2] * transport1[2]
				+ lightPrt.r[2][0] * transport2[0] + lightPrt.r[2][1] * transport2[1] + lightPrt.r[2][2] * transport2[2];

    outColor.g = lightPrt.g[0][0] * transport0[0] + lightPrt.g[0][1] * transport0[1] + lightPrt.g[0][2] * transport0[2]
				+ lightPrt.g[1][0] * transport1[0] + lightPrt.g[1][1] * transport1[1] + lightPrt.g[1][2] * transport1[2]
				+ lightPrt.g[2][0] * transport2[0] + lightPrt.g[2][1] * transport2[1] + lightPrt.g[2][2] * transport2[2];

	outColor.b = lightPrt.b[0][0] * transport0[0] + lightPrt.b[0][1] * transport0[1] + lightPrt.b[0][2] * transport0[2]
				+ lightPrt.b[1][0] * transport1[0] + lightPrt.b[1][1] * transport1[1] + lightPrt.b[1][2] * transport1[2]
				+ lightPrt.b[2][0] * transport2[0] + lightPrt.b[2][1] * transport2[1] + lightPrt.b[2][2] * transport2[2];
}