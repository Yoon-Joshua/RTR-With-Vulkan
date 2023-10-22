#pragma once

#include <glm/glm.hpp>
#include "glfw_context.h"
#include "timer.h"
#include "context.h"
#include "swapchain.h"
#include "command_buffer.h"

struct MVP {
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
	alignas(16) glm::vec3 eye;
};

struct Light {
	alignas(16) glm::vec3 pos{0.0f, 0.0f, 0.0f};
	alignas(16) glm::vec3 intensity{0.0f, 0.0f, 0.0f};
};

class Application {
public:
	Application(GlfwContext* glfwContext_ );
	~Application();
	virtual void prepare() = 0;
	virtual void mainLoop(Timer& timer);
	virtual void cleanup() = 0;
protected:
	virtual void drawFrame() = 0;
	virtual void resizeWindow() = 0;

	GlfwContext* glfwContext{ nullptr };
	Context context;
	SwapChain swapchain;
	CommandBuffer commandBuffer;

	VkFormat depthFormat{ VK_FORMAT_UNDEFINED };
};