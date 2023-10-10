#version 460

layout(location = 0) in vec4 inNormal;
layout(location = 1) in vec4 inPosition;
layout(location = 2) in vec4 inLightPos;
layout(location = 3) in vec2 inTexCoord;
layout(location = 4) in vec4 smCoord;

layout(location = 0) out vec4 outColor;

layout(binding = 2, set = 0) uniform LightInfo{
	vec4 pos;
	vec4 intensity;
}lightInfo;

layout(binding = 0, set = 1) uniform sampler2D texSampler;
layout(binding = 3, set = 0) uniform sampler2D shadowmapSampler;

void main(){
	vec4 color = texture(texSampler, inTexCoord);

	vec3 lightDir = inLightPos.xyz - inPosition.xyz;
	lightDir = normalize(lightDir);

	float albedo = 1.0;
	float ambient = 0.05;
	float cosine = dot(lightDir, inNormal.xyz);
	if(cosine < 0) {
		cosine = 0;
	}

	vec4 smValue = texture(shadowmapSampler, smCoord.xy);

	if(smValue.z < gl_FragCoord.z){
		albedo = 1.0;
	}

	outColor = vec4(albedo * cosine * lightInfo.intensity.xyz, 1.0) * color + ambient * color;
}