#version 460

layout(binding = 0, set = 0) uniform Model {
	mat4 transform;
} model;

layout(binding = 1, set = 1) uniform ViewProj{
	mat4 transform;
}view_proj;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 transport0;
layout(location = 3) in vec3 transport1;
layout(location = 4) in vec3 transport2;

layout(location = 0) out vec3 outUVW;

void main(){
	// keep the depth be 1.0 always
	gl_Position = view_proj.transform * model.transform * vec4(inPosition, 1.0);
	outUVW = inPosition;
}