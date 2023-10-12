#include "camera.h"

Camera::Camera() :fovy(75), aspect(1), near(0.01), far(10000), up(vec3(0.0, 1.0, 0.0)), eye(vec3(30.0, 30.0, 30.0)), centre(vec3(0.0, 0.0, 0.0)) {}

mat4 Camera::lookAt() { return glm::lookAt(eye, centre, up); }

mat4 Camera::perspective() { 
	mat4 mPer = glm::perspective(glm::radians(fovy), aspect, near, far);
	mPer[1][1] *= -1;
	return mPer;
}

void Camera::setFovy(float f) { fovy = f; }

void Camera::setAspect(float a) { aspect = a; }

void Camera::setPosition(float p) { eye = p * (1.0f + eye / 1000.0f); }
