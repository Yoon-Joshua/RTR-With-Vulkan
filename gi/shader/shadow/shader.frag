#version 460

layout(location = 0) in vec3 lightFromEye;
layout(location = 1) in vec3 normalFromEye;
layout(location = 2) in vec3 posFromEye;

layout(location = 0) out vec4 outColor;

layout(binding = 1, set = 0) uniform Light{
	vec3 pos;
	vec3 intensity;
}light;

vec3 ambient = vec3(0.1, 0.1, 0.1);
vec3 albedo = vec3(1.0, 1.0, 1.0);

void main(){
	vec3 lightDir = normalize(lightFromEye - posFromEye);
	vec3 normal = normalize(normalFromEye);
	float cos = max(dot(lightDir, normal), 0.0);
	//cos = 1.0;
	outColor = vec4((cos * albedo + ambient) * light.intensity, 1.0);
}
