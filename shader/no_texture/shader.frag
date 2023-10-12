#version 450

layout(location = 0) in vec3 inNormal;
layout(location = 1) in vec3 inLightPos;
layout(location = 2) in vec3 inPosition;
layout(location = 3) in vec4 smCoord;

layout(location = 0) out vec4 outColor;

layout(binding = 2, set = 0) uniform LightInfo{
	vec4 pos;
	vec4 intensity;
}lightInfo;

layout(binding = 3, set = 0) uniform sampler2D smSampler;

void main(){
	vec3 lightDir = inLightPos - inPosition;
	lightDir = normalize(lightDir);

	float albedo = 0.5;
	float cosine = dot(lightDir, inNormal);
	if(cosine < 0) {
		cosine = 0;
	}

	float z = smCoord.z / smCoord.w;
	vec4 smc = (smCoord / smCoord.w + 1.0) / 2.0;

	vec2 texCoord = vec2(smc.s, smc.t);
	vec4 smValue = texture(smSampler, texCoord);

	if(smValue.x + 0.1 < z && smc.x <= 0.9 && smc.x >= 0.1 && smc.y >= 0.1 && smc.y <= 0.9){
		albedo = 0.1;
	}

	outColor = vec4(albedo * cosine * lightInfo.intensity.xyz, 1.0);
	//outColor = smValue;

	
	//vec4 smc = smCoord;

	/*
	if(smCoord.y / smCoord.w > 1.0){
		outColor = vec4(1.0,0.0,0.0,1.0);
	}
	else if(smCoord.y /smCoord.w <0.0){
		outColor = vec4(0.0,1.0,0.0,1.0);
	}
	else{
		outColor = vec4(0.0,0.0,0.0,1.0);
	}
	*/
}