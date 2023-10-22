#version 460
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 outPosition;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec4 screenCoord;

layout(binding = 0, set = 0) uniform MVP{
	mat4 model;
	mat4 view;
	mat4 proj;
}mvp;

layout(binding = 0, set = 1) uniform LightMVP{
	mat4 model;
	mat4 view;
	mat4 proj;
}lightmvp;

void main(){
	gl_Position = mvp.proj * mvp.view * mvp.model * vec4(inPosition, 1.0);
	outPosition = (mvp.model * vec4(inPosition, 1.0)).xyz;
	outNormal = normalize((mvp.model * vec4(inNormal, 0.0)).xyz);
	//outNormal = (mvp.model * vec4(inNormal, 0.0)).xyz;
	screenCoord = gl_Position;
}