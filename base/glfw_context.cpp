#include "glfw_context.h"

double GlfwContext::scroll_factor = 45;
double GlfwContext::cur_mx = 0;
double GlfwContext::cur_my = 0;
double GlfwContext::last_mx = 0;
double GlfwContext::last_my = 0;

bool GlfwContext::framebufferResized = false;

void GlfwContext::framebufferResizeCallback(GLFWwindow* window, int width, int height) {
	framebufferResized = true;
}