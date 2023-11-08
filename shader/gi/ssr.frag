#version 460

layout(location = 0) in vec4 inScreenCoord;

layout(location = 0) out vec4 outColor;

layout(binding = 0, set = 1) uniform CameraParam{
	mat4 view;
	mat4 proj;
	vec3 eye;
}cameraParam;

layout(binding = 1, set = 1) uniform Light{
	vec3 pos;
	vec3 intensity;
}light;

layout(binding = 2, set = 1) uniform LightTransform{
	mat4 transform;
}lightTransform;

// 0 shadow
// 1 normal
// 2 depth
// 3 color
// 4 worldPos
layout(binding = 3, set = 1) uniform sampler2D gBuffer[5];

#define SHADOW_MAP 0
#define NORMAL_MAP 1
#define DEPTH_MAP 2
#define COLOR_MAP 3
#define WORLD_POS_MAP 4

#define EPSILON 0.001
#define SHADOW_BIAS 0.001
#define PI 3.1415926535

bool flag = true;

float getVisibility(vec3 pos){
	vec4 shadow_coord = lightTransform.transform * vec4(pos, 1.0);
	vec2 shadow_uv = (shadow_coord / shadow_coord.w + 1.0).xy / 2.0;
	float depth = (shadow_coord / shadow_coord.w).z;
	float shadow_depth = texture(gBuffer[SHADOW_MAP], shadow_uv).x;
	if(depth > shadow_depth + SHADOW_BIAS)
		return 0.00;
	else
		return 1.0;
}

vec3 randomDir(float seed){
	float phi = (fract(sin(seed)*100.0) + 1.0) * PI;
	float theta = acos((fract(sin(phi)*1000.0)));
	return vec3(sin(theta) * cos(phi), cos(theta), sin(theta) * sin(phi));
}

vec3 rayMarch(vec3 position, vec3 reflectDir){
	vec3 photon = position;
	vec4 power = vec4(0.0, 0.0, 0.0, 0.0);
	for(int i = 0; i < 200; ++i) {
		photon = photon + 0.02 * reflectDir;
		vec4 screen_coord = cameraParam.proj * cameraParam.view * vec4(photon, 1.0);
		screen_coord = screen_coord / screen_coord.w;
		if(screen_coord.z + EPSILON > 1.0){
			return vec3(0,0,0);
		}
		if(screen_coord.z < EPSILON){
			return vec3(0,0,0);
		}
		vec2 screenUV = (screen_coord.xy + 1.0) / 2;
		if(screenUV.x + EPSILON > 1.0 || screenUV.x < EPSILON || screenUV.y + EPSILON > 1.0 || screenUV.y < EPSILON){
			return vec3(0,0,0);
		}
		
		float depth = (texture(gBuffer[DEPTH_MAP], screenUV)).x;
		vec3 lightDir = normalize(light.pos - photon);
		vec3 normal = (texture(gBuffer[NORMAL_MAP], screenUV)).xyz;
		if(depth < screen_coord.z){
			float visibility = getVisibility(photon);
			flag = false;
			return visibility * texture(gBuffer[COLOR_MAP], screenUV).xyz * max(dot(lightDir, normal), 0);
		}
	}
	return vec3(0, 0, 0);
}

#define SAMPLE_NUM 10

void main(){
	vec4 screenCoord = inScreenCoord / inScreenCoord.w;
	vec2 screenUV = (screenCoord.xy + 1.0) / 2.0;

	vec3 normal = (texture(gBuffer[NORMAL_MAP], screenUV)).xyz;

	vec3 position = (texture(gBuffer[WORLD_POS_MAP], screenUV)).xyz;

	vec3 viewDir = normalize(cameraParam.eye - position);
	vec3 reflectDir = normalize(reflect(-viewDir, normal));
	vec3 lightDir = normalize(light.pos - position);

	float visibility = getVisibility(position);
	vec3 albedo = (texture(gBuffer[COLOR_MAP], screenUV)).xyz;
	vec3 L = visibility * albedo * max(dot(lightDir, normal), 0) * light.intensity;
	
	vec3 L_indirect = vec3(0, 0, 0);
	
	L_indirect += rayMarch(position, reflectDir);

	vec3 ambient = vec3(0.05, 0.05, 0.05);
	outColor = vec4(L + albedo * (0.5 *L_indirect + ambient), 1.0);
}