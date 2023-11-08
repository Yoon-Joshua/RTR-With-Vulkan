#version 460

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec4 outPosition;

layout(binding = 0, set = 0) uniform Model{
	mat4 transform;
}model;

layout(binding = 0, set = 1) uniform UniformObj{
	mat4 transform;
}lightTransform;

void main(){
	outPosition = lightTransform.transform * model.transform * vec4(inPosition, 1.0);
	gl_Position = outPosition;
}