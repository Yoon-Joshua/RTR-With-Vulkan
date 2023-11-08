#version 460

layout(binding = 1, set = 0) uniform UBO {
	mat4 transform;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 outUVW;

void main(){
	// keep the depth be 1.0 always
	gl_Position = ubo.transform * vec4(inPosition, 1.0);
	outUVW = inPosition;
}