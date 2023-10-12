#version 460

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 outNormal;
layout(location = 1) out vec3 outPosition;
layout(location = 2) out vec4 smCoord;

layout(binding = 0, set = 0) uniform UniformObj{
	mat4 model;
	mat4 view;
	mat4 proj;
}ubo;

layout(binding = 1, set = 0) uniform UboLight{
	mat4 model;
	mat4 view;
	mat4 proj;
}uboLight;

void main(){
	gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);

	outNormal = (ubo.model * vec4(inNormal, 0.0)).xyz;

	outPosition = (ubo.model * vec4(inPosition, 1.0)).xyz;

	vec4 smc = uboLight.proj * uboLight.view * uboLight.model * vec4(inPosition, 1.0);
	smCoord = smc / smc.w;
}