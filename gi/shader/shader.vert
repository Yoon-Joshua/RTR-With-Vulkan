#version 460
layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoord;

layout(location = 0) out vec2 uv;
layout(location = 1) out vec3 lightFromEye;
layout(location = 2) out vec3 normalFromEye;
layout(location = 3) out vec3 posFromEye;
layout(location = 4) out vec4 smCoord;

layout(binding = 0, set = 0) uniform UBO{
	mat4 model;
	mat4 view;
	mat4 proj;
}ubo;

layout(binding = 2, set = 0) uniform Light{
	vec3 pos;
	vec3 intensity;
}light;

layout(binding = 4, set = 0) uniform UBOLight{
	mat4 model;
	mat4 view;
	mat4 proj;
}uboLight;

void main(){
	gl_Position = ubo.proj * ubo.view * ubo.model * vec4(pos, 1.0);
	uv = texCoord;
	lightFromEye = (ubo.view * vec4(light.pos, 1.0)).xyz;
	posFromEye = (ubo.view * ubo.model * vec4(pos, 1.0)).xyz;
	normalFromEye = mat3(transpose(inverse(ubo.view * ubo.model))) * normal;
	smCoord = uboLight.proj * uboLight.view * ubo.model * vec4(pos, 1.0);
}
