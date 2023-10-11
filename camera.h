#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
using mat4 = glm::mat4;
using vec3 = glm::vec3;

class Camera {

public:
	Camera();
	mat4 lookAt();
	mat4 perspective();
	void setFovy(float f);
	void setAspect(float a);

private:
	float fovy;
	float aspect;
	float near;
	float far;

	vec3 up;
	vec3 eye;
	vec3 centre;
};