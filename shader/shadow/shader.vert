#version 460

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec4 outPosition;

layout(binding = 0) uniform UniformObj{
	mat4 model;
	mat4 view;
	mat4 proj;
}ubo;

void main(){
	outPosition = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
	gl_Position = outPosition;
}