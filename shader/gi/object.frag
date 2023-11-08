#version 460

layout(location = 0) in vec2 uv;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inPosition;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outWorldPos;
layout(location = 2) out vec4 outNormal;

layout(binding = 1, set = 1) uniform sampler2D texSampler;

vec3 albedo = vec3(1.0, 1.0, 1.0);

void main(){
	vec3 normal = normalize(inNormal);

	outColor = vec4(albedo, 1.0) * texture(texSampler, uv);
	outNormal = vec4(normal, 1.0);
	outWorldPos = vec4(inPosition, 1.0);
}
