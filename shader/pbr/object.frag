#version 460

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform Constant{
	mat4 model;
	vec2 roughness;
}constant;

layout(binding = 0, set = 0) uniform UBO{
    mat4 view;
    mat4 proj;
    vec3 camera;
}ubo;

layout(binding = 1, set = 0) uniform sampler2D energyAvg;
layout(binding = 2, set = 0) uniform sampler2D energy;

#define PI 3.14159265

vec3 lightDir = vec3(0.0, 1.0, 1.0);
vec3 lightIntensity = 1.0 * vec3(1.0, 1.0, 1.0);

float DistributionGGX(vec3 N, vec3 H, float roughness) {
	float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0f);
    float NdotH2 = NdotH * NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / max(denom, 0.0001f);
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float a = roughness;
    float k = (a * a) / 2.0f;

    float nom = NdotV;
    float denom = NdotV * (1.0f - k) + k;

    return nom / denom;
}

float GeometrySmith(float roughness, float HdotV, float HdotL) {
    float ggx2 = GeometrySchlickGGX(HdotV, roughness);
    float ggx1 = GeometrySchlickGGX(HdotL, roughness);

    return ggx1 * ggx2;
}

vec3 fresnelSchlick(vec3 F0, vec3 V, vec3 H) {
    // TODO: To calculate Schlick F here
    return F0 + (1.0 - F0) * pow(clamp(1.0 - max(dot(H, V), 0.0), 0.0, 1.0), 5.0);
    return vec3(1.0);
}

vec3 AverageFresnel(vec3 r, vec3 g) {
    return vec3(0.087237) + 0.0230685*g - 0.0864902*g*g + 0.0774594*g*g*g
           + 0.782654*r - 0.136432*r*r + 0.278708*r*r*r
           + 0.19744*g*r + 0.0360605*g*g*r - 0.2586*g*r*r;
}

vec3 MultiScatterBRDF(float NdotL, float NdotV) {
	float roughness = constant.roughness.x;
	vec3 albedo = pow(vec3(0.796, 0.62, 0.482), vec3(2.2));
	
	vec3 E_o = texture(energy, vec2(NdotL, roughness)).xyz;
	vec3 E_i = texture(energy, vec2(NdotV, roughness)).xyz;

	vec3 E_avg = texture(energyAvg, vec2(0.0, roughness)).xyz;
	// copper
	vec3 edgetint = vec3(0.827, 0.792, 0.678);
	vec3 F_avg = AverageFresnel(albedo, edgetint);

	// TODO: To calculate fms and missing energy here
	vec3 F_ms = (1.0 - E_o) * (1.0 - E_i) / (PI * (1.0 - E_avg));
	vec3 F_add = F_avg * E_avg / (1.0 - F_avg * (1.0 - E_avg));
	
	return F_ms * F_add;
}

void main(){
	float roughness = constant.roughness.x;
    vec3 albedo = pow(vec3(0.796, 0.62, 0.482), vec3(2.2));

    vec3 N = normalize(inNormal);
    vec3 V = normalize(ubo.camera - inPosition);
    float NdotV = max(dot(N, V), 0.0);
 
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, vec3(1.0));

    vec3 Lo = vec3(0.0);
    
    vec3 L = normalize(lightDir);
    vec3 H = normalize(V + L);
    float NdotL = max(dot(N, L), 0.0); 

    float NDF = DistributionGGX(N, H, roughness);   
    float G   = GeometrySmith(roughness, NdotV, NdotL); 
	float HdotL = max(dot(H, L), 0.0);
	float HdotV = max(dot(H, V), 0.0);
    vec3 F = fresnelSchlick(F0, V, H);
      
    vec3 numerator = NDF * G * F; 
    float denominator = max((4.0 * NdotL * NdotV), 0.001);
    vec3 BRDF = numerator / denominator;
	if(constant.roughness.y > 0.5)
		BRDF += MultiScatterBRDF(NdotL, NdotV);

    Lo += BRDF * lightIntensity * NdotL;
    vec3 color = Lo;

    //color = color / (color + vec3(1.0));
    //color = pow(color, vec3(1.0/2.2)); 
    outColor = vec4(color, 1.0);
}