#version 460

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 outNormal;
layout(location = 1) out vec3 outPosition;
layout(location = 2) out vec2 outTexCoord;
layout(location = 3) out vec4 smCoord;

layout(binding = 0, set = 0) uniform Model{
	mat4 model;
} model;

layout(binding = 0, set = 1) uniform CameraParam{
	mat4 view;
	mat4 proj;
	vec3 eye;
}cameraParam;

layout(binding = 1, set = 1) uniform Transform{
	mat4 transform;
}lightTransform;

void main(){
	gl_Position = cameraParam.proj * cameraParam.view * model.model * vec4(inPosition, 1.0);
	outNormal = (model.model * vec4(inNormal, 0.0)).xyz;
	outPosition = (model.model * vec4(inPosition, 1.0)).xyz;
	outTexCoord = inTexCoord;
	smCoord = lightTransform.transform * model.model * vec4(inPosition, 1.0);
}