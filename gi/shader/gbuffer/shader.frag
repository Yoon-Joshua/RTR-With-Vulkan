#version 460

layout(location = 0) in vec2 uv;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inPosition;
layout(location = 3) in vec4 smCoord;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outWorldPos;
layout(location = 2) out vec4 outNormal;

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
	vec3 lightDir = normalize(light.pos - inPosition);
	vec3 normal = normalize(inNormal);
	float cos = max(dot(lightDir, normal), 0.0);

	float z = smCoord.z / smCoord.w;
	vec2 smUV = ((smCoord / smCoord.w + 1.0) / 2.0).xy;
	float smDepth = (texture(shadowSampler, smUV)).r;
	if(smDepth + 0.005 < z){
		outColor = vec4(ambient * light.intensity, 1.0) * texture(texSampler, uv);
	}
	else{
		outColor = vec4((ambient + cos * albedo) * light.intensity, 1.0) * texture(texSampler, uv);
	}
	outNormal = vec4(normal, 1.0);
	outWorldPos = vec4(inPosition, 1.0);
}
