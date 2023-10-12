#include "application.h"
#include "timer.h"
#include "glfw_context.h"

int main() {
	Timer timer;
	Application app;
	GlfwContext wnd;
	
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	wnd.window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
	glfwSetWindowUserPointer(wnd.window, &wnd);
	glfwSetFramebufferSizeCallback(wnd.window, GlfwContext::framebufferResizeCallback);
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	wnd.glfwExtensions.assign(glfwExtensions, glfwExtensions + glfwExtensionCount);

	app.init(&wnd);
	app.mainLoop(timer);
	app.cleanup();
}