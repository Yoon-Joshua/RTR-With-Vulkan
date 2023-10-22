#version 460

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec4 inScreenCoord;

layout(location = 0) out vec4 outColor;

layout(binding = 0, set = 0) uniform MVP{
	mat4 model;
	mat4 view;
	mat4 proj;
	vec3 eye;
}mvp;

layout(binding = 0, set = 1) uniform LightMVP{
	mat4 model;
	mat4 view;
	mat4 proj;
	vec3 pos;
}lightmvp;

layout(binding = 1, set = 1) uniform Light{
	vec3 pos;
	vec3 intensity;
}light;

// 0 shadow
// 1 normal
// 2 depth
// 3 color
layout(binding = 0, set = 2) uniform sampler2D gBuffer[4];

#define SHADOW_MAP 0
#define NORMAL_MAP 1
#define DEPTH_MAP 2
#define COLOR_MAP 3

#define EPSILON 0.0001
#define SHADOW_BIAS 0.0005

vec3 ambient = vec3(0.1, 0.1, 0.1);

float getVisibility(vec3 pos){
	vec4 shadow_coord = lightmvp.proj * lightmvp.view * vec4(pos, 1.0);
	vec2 shadow_uv = (shadow_coord / shadow_coord.w + 1.0).xy / 2.0;
	float depth = (shadow_coord / shadow_coord.w).z;
	float shadow_depth = texture(gBuffer[SHADOW_MAP], shadow_uv).x;
	if(depth > shadow_depth + SHADOW_BIAS)
		return 0.00;
	else
		return 1.0;
}

vec3 rayMarch(vec3 reflectDir){
	vec3 photon = inPosition;
	vec4 power = vec4(0.0, 0.0, 0.0, 0.0);
	for(int i = 0; i < 200; ++i) {
		photon = photon + 0.03 * reflectDir;
		vec4 screen_coord = mvp.proj * mvp.view * vec4(photon, 1.0);
		screen_coord = screen_coord / screen_coord.w;
		if(screen_coord.z + EPSILON > 1.0) break;
		if(screen_coord.z < EPSILON) break;
		vec2 screenUV = (screen_coord.xy + 1.0) / 2;
		if(screenUV.x + EPSILON > 1.0 || screenUV.x < EPSILON || screenUV.y + EPSILON > 1.0 || screenUV.y < EPSILON) break;
		
		float depth = (texture(gBuffer[DEPTH_MAP], screenUV)).x;
		vec3 lightDir = normalize(light.pos - photon);
		vec3 normal = (texture(gBuffer[NORMAL_MAP], screenUV)).xyz;
		if(depth + EPSILON < screen_coord.z){
			return getVisibility(photon) * texture(gBuffer[COLOR_MAP], screenUV).xyz * max(dot(lightDir, normal), 0);
		}
	}
	return vec3(0.0, 0.0, 0.0);
}

void main(){
	vec3 normal = normalize(inNormal);
	vec3 viewDir = normalize(mvp.eye - inPosition);
	vec3 reflectDir = normalize(reflect(-viewDir, normal));
	vec3 lightDir = normalize(light.pos - inPosition);

	vec4 screenCoord = inScreenCoord / inScreenCoord.w;
	vec2 screenUV = (screenCoord.xy + 1.0) / 2.0;

	float visibility = getVisibility(inPosition);
	vec3 L = visibility * (texture(gBuffer[COLOR_MAP], screenUV)).xyz * max(dot(lightDir, inNormal), 0) * light.intensity;
	L += ambient * (texture(gBuffer[COLOR_MAP], screenUV)).xyz;

	vec3 L_indirect = rayMarch(reflectDir);

	outColor = vec4(L + (texture(gBuffer[COLOR_MAP], screenUV)).xyz * L_indirect, 1.0);
}