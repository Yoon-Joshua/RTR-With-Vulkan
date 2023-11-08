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

layout(binding = 3, set = 1) uniform sampler2D smSampler;

float bias = 0.001;
float ambient = 0.00;
float albedo = 0.5;

#define NUM_SAMPLES 30
#define PI 3.141592653589793

// used for PCSS
#define LIGHT_SIZE_UV 0.02
#define NEAR_PLANE 0.2

vec2 poissonDisk[NUM_SAMPLES];

float rand_1to1(float x ) { 
  // -1 -1
  return fract(sin(x)*10000.0);
}

float rand_2to1(vec2 uv ) { 
	// 0 - 1
	const float a = 12.9898, b = 78.233, c = 43758.5453;
	float dt = dot( uv.xy, vec2( a,b ) ), sn = mod( dt, PI );
	return fract(sin(sn) * c);
}

void uniformDiskSamples( const in vec2 randomSeed ) {

  float randNum = rand_2to1(randomSeed);
  float sampleX = rand_1to1( randNum ) ;
  float sampleY = rand_1to1( sampleX ) ;

  float angle = sampleX * PI * 2;
  float radius = sqrt(sampleY);

  for( int i = 0; i < NUM_SAMPLES; i ++ ) {
    poissonDisk[i] = vec2( radius * cos(angle) , radius * sin(angle)  );

    sampleX = rand_1to1( sampleY ) ;
    sampleY = rand_1to1( sampleX ) ;

    angle = sampleX * PI * 2;
    radius = sqrt(sampleY);
  }
}

float PCF(vec2 uv, float z, float filterRadiusUV){
	float sum = 0.0;
	for(int i = 0; i < NUM_SAMPLES; i++) {
		vec2 sampleuv = uv + poissonDisk[i] * filterRadiusUV;
		float shadowMapDepth = (texture(smSampler, sampleuv)).x;
		if(z > shadowMapDepth + bias ){
			sum += 0.0;
		} else {
			sum += 1.0;
		}
	}
	return sum / NUM_SAMPLES;
}

float findBlocker( sampler2D shadowMap,  vec2 uv, float z ) {

	float searchWidth = LIGHT_SIZE_UV * z / (z + NEAR_PLANE);

    float blockerSum = 0.0;
    float numBlockers = 0.0;
 
	for (int i = 0; i < NUM_SAMPLES; i++) {
		vec2 sampleuv = uv + poissonDisk[i] * searchWidth;
		float shadowMapDepth = (texture(shadowMap, sampleuv)).x;
		if(shadowMapDepth < z) {
			blockerSum += shadowMapDepth;
			numBlockers++;
		}
	}

	if(numBlockers == 0.0) { 
		return z;
	}
	return blockerSum / numBlockers;
}

float PCSS(vec2 uv, float z){
	// STEP 1: avgblocker depth

	float avgBlockerDepth = findBlocker(smSampler, uv, z);

	// STEP 2: penumbra size
	float penumbraRatio = (z - avgBlockerDepth) / (avgBlockerDepth + NEAR_PLANE);

	// STEP 3: filtering
	//float filterRadiusUV = penumbraRatio * 0.01;
	float filterRadiusUV = penumbraRatio * LIGHT_SIZE_UV;
	return PCF(uv, z, filterRadiusUV);
}

float hardShadow(){
	vec4 smc = smCoord / smCoord.w;
	float z = smc.z;
	vec2 coord = (smc.xy + 1.0) / 2.0;
	vec4 value = texture(smSampler, coord);
	if(z > value.r + bias && coord.x <= 0.9 && coord.x >= 0.1 && coord.y >= 0.1 && coord.y <= 0.9){
		return 0.0;
	}
	else {
		return 1.0;
	}
}

void main(){
	vec3 lightDir = light.pos - inPosition;
	lightDir = normalize(lightDir);

	float cosine = dot(lightDir, normalize(inNormal));
	if(cosine < 0) {
		cosine = 0;
	}

	vec4 smc = smCoord / smCoord.w;
	float z = smc.z;
	vec2 uv = (smc.xy + 1.0) / 2.0;
	uniformDiskSamples(uv);

	float visibility;
	//visibility = hardShadow();
	//visibility = PCF(uv, z, 0.001);
	visibility = PCSS(uv, z);

	outColor = visibility * vec4((albedo * cosine + ambient) * light.intensity.xyz, 1.0);
}