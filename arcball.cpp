#include "arcball.h"
#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

double ArcBall::cur_mx = 0;
double ArcBall::cur_my = 0;
double ArcBall::last_mx = 0;
double ArcBall::last_my = 0;
bool ArcBall::arcball_on = false;
float ArcBall::scroll_factor = 45;
vec3 ArcBall::last_point = vec3(0.0, 0.0, 0.0);
mat4 ArcBall::old = mat4(1.0);

void ArcBall::init(GLFWwindow* window) {
	this->window = window;
	glfwGetFramebufferSize(window, &width, &height);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
}

mat4 ArcBall::get() {
	if (arcball_on) {
		glfwGetCursorPos(window, &cur_mx, &cur_my);
		if (cur_mx == last_mx && cur_my == last_my)
			return old;
		float x = cur_mx / width * 2 - 1;
		float y = cur_my / height * 2 - 1;
		float l = x * x + y * y;
		if (l > 1) {
			x = x / sqrt(l);
			y = y / sqrt(l);
			l = 1;
		}
		float z = -sqrt(1 - l);
		vec3 cur_point = vec3(x, y, z);
		vec3 axis = glm::cross(cur_point, last_point);
		float angle = acos(glm::dot(cur_point, last_point));
		return glm::rotate(mat4(1.0f), angle, axis) * old;
	}
	else {
		return old;
	}
}

void ArcBall::mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		arcball_on = true;
		glfwGetCursorPos(window, &last_mx, &last_my);
		int w, h;
		glfwGetFramebufferSize(window, &w, &h);
		float x = last_mx / w * 2 - 1;
		float y = last_my / h * 2 - 1;
		float l = x * x + y * y;
		if (l > 1) {
			x = x / sqrt(l);
			y = y / sqrt(l);
			l = 1;
		}
		float z = -sqrt(1 - l);
	    last_point = vec3(x, y, z);
	}
	else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
		arcball_on = false;
		glfwGetCursorPos(window, &cur_mx, &cur_my);
		if (cur_mx != last_mx || cur_my != last_my) {
			int w, h;
			glfwGetFramebufferSize(window, &w, &h);
			float x = cur_mx / w * 2 - 1;
			float y = cur_my / h * 2 - 1;
			float l = x * x + y * y;
			if (l > 1) {
				x = x / sqrt(l);
				y = y / sqrt(l);
				l = 1;
			}
			float z = -sqrt(1 - l);
			vec3 cur_point = vec3(x, y, z);
			vec3 axis = glm::cross(cur_point, last_point);
			float angle = acos(glm::dot(cur_point, last_point));
			old = glm::rotate(mat4(1.0f), angle, axis) * old;
		}
	}
	else {
		arcball_on = false;
	}

}

void ArcBall::cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {

}

void ArcBall::scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
	scroll_factor += yoffset;
	scroll_factor = std::clamp(scroll_factor, 10.0f, 80.0f);
}