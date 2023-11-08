#version 460

layout(location = 0) in vec3 inNormal;
layout(location = 1) in vec3 inPosition;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec4 smCoord;

layout(location = 0) out vec4 outColor;

layout(binding = 2, set = 1) uniform Light{
	vec3 pos;
	vec3 intensity;
}light;

layout(binding = 3, set = 1) uniform sampler2D shadowmapSampler;

layout(binding = 0, set = 2) uniform sampler2D texSampler;

float bias = 0.002;

void main(){
	vec4 color = texture(texSampler, inTexCoord);

	vec3 lightDir = light.pos - inPosition;
	lightDir = normalize(lightDir);

	float albedo = 0.5;
	float ambient = 0.1;
	float cosine = dot(lightDir, normalize(inNormal));
	if(cosine < 0) {
		cosine = 0;
	}

	vec4 smc = smCoord / smCoord.w;
	vec4 smValue = texture(shadowmapSampler, (smc.xy + 1.0) / 2.0);
	if(smc.z > smValue.x + bias) {
		albedo = 0.0;
	}

	outColor = vec4(albedo * cosine * light.intensity.xyz, 1.0) * color + ambient * color;
}