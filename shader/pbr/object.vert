#version 460

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 outPosition;
layout(location = 1) out vec3 outNormal;

layout(binding = 0, set = 0) uniform UBO{
	mat4 view;
	mat4 proj;
	vec3 camera;
}ubo;

layout(push_constant) uniform Constant{
	mat4 model;
	float roughness;
}constant;

void main(){
	gl_Position = ubo.proj * ubo.view * constant.model * vec4(inPosition, 1.0);
	outPosition = (constant.model * vec4(inPosition, 1.0)).xyz;
	outNormal = inNormal;
}