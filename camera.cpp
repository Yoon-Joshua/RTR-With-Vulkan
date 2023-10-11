#include "camera.h"

Camera::Camera() :fovy(45), aspect(1), near(0.1), far(100), up(vec3(0.0, 1.0, 0.0)), eye(vec3(8.0, 5.0, 8.0)), centre(vec3(0.0, 0.0, 0.0)) {}

mat4 Camera::lookAt() { return glm::lookAt(eye, centre, up); }

mat4 Camera::perspective() { 
	mat4 mPer = glm::perspective(glm::radians(fovy), aspect, near, far);
	mPer[1][1] *= -1;
	return mPer;
}

void Camera::setFovy(float f) { fovy = f; }

void Camera::setAspect(float a) { aspect = a; }

