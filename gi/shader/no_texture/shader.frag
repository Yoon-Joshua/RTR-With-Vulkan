#version 460

layout(location = 0) in vec2 uv;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inPosition;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outWorldPos;
layout(location = 2) out vec4 outNormal;

layout(binding = 1, set = 1) uniform Light{
	vec3 pos;
	vec3 intensity;
}light;

vec3 ambient = vec3(0.0, 0.0, 0.0);
vec3 albedo = vec3(0.5, 0.5, 0.5);

void main(){
	vec3 lightDir = normalize(light.pos - inPosition);
	vec3 normal = normalize(inNormal);
	float cos = max(dot(lightDir, normal), 0.0);

	//outColor = vec4((ambient + cos * albedo) * light.intensity, 1.0);
	outColor = vec4(albedo, 1.0);
	outNormal = vec4(normal, 1.0);
	outWorldPos = vec4(inPosition, 1.0);
}
