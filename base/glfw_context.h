#pragma once

#include <GLFW/glfw3.h>
#include <vector>

struct GlfwContext {
	GLFWwindow* window{ nullptr };
	std::vector<const char*> glfwExtensions;
	
	static double scroll_factor;
	static double cur_mx, cur_my, last_mx, last_my;

	static bool framebufferResized;
	static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
};