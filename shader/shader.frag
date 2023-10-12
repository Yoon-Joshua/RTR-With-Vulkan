#version 460

layout(location = 0) in vec3 inNormal;
layout(location = 1) in vec3 inPosition;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec4 smCoord;

layout(location = 0) out vec4 outColor;

layout(binding = 2, set = 0) uniform LightInfo{
	vec3 pos;
	vec3 intensity;
}lightInfo;

layout(binding = 0, set = 1) uniform sampler2D texSampler;
layout(binding = 3, set = 0) uniform sampler2D shadowmapSampler;

void main(){
	vec4 color = texture(texSampler, inTexCoord);

	vec3 lightDir = lightInfo.pos - inPosition;
	lightDir = normalize(lightDir);

	float albedo = 1.0;
	float ambient = 0.00;
	float cosine = dot(lightDir, normalize(inNormal));
	if(cosine < 0) {
		cosine = 0;
	}

	cosine = pow(cosine, 4);

	vec4 smValue = texture(shadowmapSampler, smCoord.xy);

	outColor = vec4(albedo * cosine * lightInfo.intensity.xyz, 1.0) * color + ambient * color;
}