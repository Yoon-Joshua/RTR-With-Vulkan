#include "global.h"
#include <iostream>
#include "timer.h"
#include "glfw_context.h"

#ifdef PCSS
#include "application.h"
#elif defined PRT
#include "prt/prt.h"
#endif

int main() {
	Timer timer;
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
#ifdef PCSS
	Application app;
	app.init(&wnd);
	app.mainLoop(timer);
	app.cleanup();
#elif defined PRT
	PRTApplication app;
	app.init(&wnd);
	app.prepare();
	app.mainLoop(timer);
	app.cleanup();
#endif

}