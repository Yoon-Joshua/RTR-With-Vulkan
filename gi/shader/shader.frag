#version 460

layout(location = 0) in vec2 uv;
layout(location = 1) in vec3 lightFromEye;
layout(location = 2) in vec3 normalFromEye;
layout(location = 3) in vec3 posFromEye;
layout(location = 4) in vec4 smCoord;

layout(location = 0) out vec4 outColor;

layout(binding = 1, set = 0) uniform sampler2D texSampler;

layout(binding = 2, set = 0) uniform Light{
	vec3 pos;
	vec3 intensity;
}light;

layout(binding = 3, set = 0) uniform sampler2D shadowSampler;

layout(binding = 4, set = 0) uniform UBOLight{
	mat4 model;
	mat4 view;
	mat4 proj;
}uboLight;

vec3 ambient = vec3(0.0, 0.0, 0.0);
vec3 albedo = vec3(1.0, 1.0, 1.0);
vec3 specular = vec3(1.0, 1.0, 1.0);

void main(){
	vec3 lightDir = normalize(lightFromEye - posFromEye);
	vec3 normal = normalize(normalFromEye);
	vec3 viewDir = normalize(-posFromEye);
	vec3 h = normalize(lightDir + viewDir);
	float cos1 = max(dot(lightDir, normal), 0.0);
	float cos2 = pow(max(dot(h, normal), 0.0), 128);

	float z = smCoord.z / smCoord.w;
	vec2 smUV = ((smCoord / smCoord.w + 1.0) / 2.0).xy;
	float smDepth = (texture(shadowSampler, smUV)).r;
	if(smDepth + 0.005 < z){
		outColor = vec4(ambient * light.intensity, 1.0) * texture(texSampler, uv);
	}
	else{
		outColor = vec4((ambient + cos1 * albedo + cos2 * specular) * light.intensity, 1.0) * texture(texSampler, uv);
	}
}
