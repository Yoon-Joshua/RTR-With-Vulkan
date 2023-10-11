#pragma once

#include <glm/glm.hpp>
#include <GLFW/glfw3.h>

using vec3 = glm::vec3;
using mat4 = glm::mat4;

class ArcBall {
public:
	void init(GLFWwindow* window);

	mat4 get();

	static double last_mx, last_my, cur_mx, cur_my;
	static float scroll_factor;
	static bool arcball_on;
	static vec3 last_point;
	static mat4 old;

private:
	static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
	static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
	static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

	GLFWwindow* window;
	int width;
	int height;
};

