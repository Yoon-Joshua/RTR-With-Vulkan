#version 460
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec4 screenCoord;

layout(binding = 0, set = 0) uniform Model{
	mat4 transform;
}model;

layout(binding = 0, set = 1) uniform CameraParam{
	mat4 view;
	mat4 proj;
	vec3 eye;
}cameraParam;

void main(){
	gl_Position = cameraParam.proj * cameraParam.view * model.transform * vec4(inPosition, 1.0);
	screenCoord = gl_Position;
}