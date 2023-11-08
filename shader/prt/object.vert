#version 460

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 transport0;
layout(location = 3) in vec3 transport1;
layout(location = 4) in vec3 transport2;

layout(location = 0) out vec2 outTexCoord;
layout(location = 1) out vec3 outColor;

layout(binding = 0, set = 0) uniform Model{
	mat4 transform;
} model;

layout(binding = 0, set = 1) uniform ViewProj{
	mat4 transform;
} view_proj;

layout(binding = 1, set = 1) uniform LightCoef {
	mat3 r;
	mat3 g;
	mat3 b;
} lightCoef;

void main(){
	gl_Position = view_proj.transform * model.transform * vec4(inPosition, 1.0);
	outTexCoord = inTexCoord;

	outColor.r = lightCoef.r[0][0] * transport0[0] + lightCoef.r[0][1] * transport0[1] + lightCoef.r[0][2] * transport0[2]
				+ lightCoef.r[1][0] * transport1[0] + lightCoef.r[1][1] * transport1[1] + lightCoef.r[1][2] * transport1[2]
				+ lightCoef.r[2][0] * transport2[0] + lightCoef.r[2][1] * transport2[1] + lightCoef.r[2][2] * transport2[2];

    outColor.g = lightCoef.g[0][0] * transport0[0] + lightCoef.g[0][1] * transport0[1] + lightCoef.g[0][2] * transport0[2]
				+ lightCoef.g[1][0] * transport1[0] + lightCoef.g[1][1] * transport1[1] + lightCoef.g[1][2] * transport1[2]
				+ lightCoef.g[2][0] * transport2[0] + lightCoef.g[2][1] * transport2[1] + lightCoef.g[2][2] * transport2[2];

	outColor.b = lightCoef.b[0][0] * transport0[0] + lightCoef.b[0][1] * transport0[1] + lightCoef.b[0][2] * transport0[2]
				+ lightCoef.b[1][0] * transport1[0] + lightCoef.b[1][1] * transport1[1] + lightCoef.b[1][2] * transport1[2]
				+ lightCoef.b[2][0] * transport2[0] + lightCoef.b[2][1] * transport2[1] + lightCoef.b[2][2] * transport2[2];
}