#version 460

layout(location = 0) in vec3 inNormal;
layout(location = 1) in vec3 inPosition;
layout(location = 2) in vec4 smCoord;

layout(location = 0) out vec4 outColor;

layout(binding = 2, set = 0) uniform LightInfo{
	vec3 pos;
	vec3 intensity;
}lightInfo;

layout(binding = 3, set = 0) uniform sampler2D smSampler;

struct Material{
	vec3 ambient;
	vec3 albedo;
};
Material material;

float bias = 0.000001;
float ambient = 0.00;
float albedo = 1.5;

void main(){
	vec3 lightDir = lightInfo.pos - inPosition;
	lightDir = normalize(lightDir);

	float cosine = dot(lightDir, normalize(inNormal));
	if(cosine < 0) {
		cosine = 0;
	}

	float z = smCoord.z;
	vec4 smc = (smCoord + 1.0) / 2.0;

	vec2 texCoord = vec2(smc.s, smc.t);
	vec4 smValue = texture(smSampler, texCoord);

	if(smc.x <= 0.9 && smc.x >= 0.1 && smc.y >= 0.1 && smc.y <= 0.9){
		albedo = 0.0;
	}

	outColor = vec4((albedo * cosine + ambient) * lightInfo.intensity.xyz, 1.0);
	outColor = smc;
}