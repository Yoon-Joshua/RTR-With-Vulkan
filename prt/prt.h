#pragma once

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <vector>
#include <array>
#include <memory>
#include "context.h"
#include "swapchain.h"
#include "image.h"
#include "common.h"
#include "command_buffer.h"
#include "renderpass.h"
#include "scene.h"
#include "pipeline.h"
#include "texture.h"
#include "timer.h"
#include "camera.h"
#include "arcball.h"
#include "glfw_context.h"

struct MVP {
	alignas(16) mat4 model;
	alignas(16) mat4 view;
	alignas(16) mat4 proj;
};

struct LightInfo {
	alignas(16) vec3 pos;
	alignas(16) vec3 intensity;
};

struct LightPRT {
	alignas(16) mat3 r;
	alignas(16) mat3 g;
	alignas(16) mat3 b;
};

/// @note Swapchain and Surface are necessary.
class PRTApplication {
public:
	void init(GlfwContext* glfwContext_);
	void prepare();
	void mainLoop(Timer& timer);
	void cleanup();

private:
	void record(uint32_t imageIndex);
	void drawFrame();
	void createSyncObjects();
	void resizeWindow();

	void updateUniformObjects(size_t index);
	/************************************************/
	void createDescriptorPool();
	void createDescriptorSetLayout();
	void createDescriptorSetLayoutSkyBox();
	void createDescriptorSet();
	void createDescriptorSetSkyBox();
	/************************************************/

	GlfwContext* glfwContext{ nullptr };
	Context context;
	Camera camera;
	Scene scene;
	SwapChain swapchain;
	Image depthImage;
	Image colorImage;
	Texture texture;
	Texture skybox;
	MVP mvp;
	LightPRT lightPrt;
	std::vector<Buffer> mvpBuffer;
	std::vector<Buffer> lightPrtBuffer;

	VkFormat depthFormat;
	Pipeline pipeline;
	Pipeline pipelineSkyBox;

	VkDescriptorSetLayout descriptorSetLayout{ VK_NULL_HANDLE };
	VkDescriptorSetLayout descriptorSetLayoutSkyBox{ VK_NULL_HANDLE };

	CommandBuffer commandBuffer;

	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;
	uint32_t currentFrame = 0;

	RenderPass renderpass;
	VkDescriptorPool descriptorPool{ VK_NULL_HANDLE };
	std::vector<VkDescriptorSet> descriptorSet;
	std::vector<VkDescriptorSet> descriptorSetSkyBox;
};

/*
* flight frame
*
* command buffer
* semaphore
* fence
* descriptor set
*
*/

