#version 460

layout(binding = 0, set = 0) uniform samplerCube samplerCubeMap;

layout(location = 0) in vec3 inUVW;

layout(location = 0) out vec4 outColor;

void main(){
	outColor = texture(samplerCubeMap, inUVW);
	//outColor = vec4(1.0, 0.0, 0.0, 1.0);
}