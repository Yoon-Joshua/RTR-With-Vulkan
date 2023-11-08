#version 460
layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoord;

layout(location = 0) out vec2 uv;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec3 outPosition;

layout(binding = 0, set = 0) uniform Model{
	mat4 transform;
}model;

layout(binding = 0, set = 1) uniform CameraParam{
	mat4 view;
	mat4 proj;
	vec3 eye;
}cameraParam;

void main(){
	gl_Position = cameraParam.proj * cameraParam.view * model.transform * vec4(pos, 1.0);
	uv = texCoord;
	outPosition = (model.transform * vec4(pos, 1.0)).xyz;
	outNormal = mat3(transpose(inverse(model.transform))) * normal;
}
