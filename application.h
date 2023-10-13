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

#define USE_ARCBALL

struct MVP {
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};

struct LightInfo {
	alignas(16) glm::vec3 pos;
	alignas(16) glm::vec3 intensity;
};

/// @note Swapchain and Surface are necessary.
class Application {
public:
	void init(GlfwContext* glfwContext_);
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

	void createDescriptorSetLayoutPhong();
	void createDescriptorSetLayoutTexture();
	void createDescriptorSetLayoutMVP();

	void createDescriptorSetsPhong();
	void createDescriptorSetsTexture();
	void createDescriptorSetsMVP();
	/************************************************/

	GlfwContext* glfwContext{ nullptr };
	Context context;
	Camera camera;
	ArcBall arcball;
	Scene scene;
	SwapChain swapchain;
	Image depthImage;
	Image colorImage;
	Texture texture;
	Image shadowDepth;
	Image shadowColor;
	Texture shadowmap;

	VkFormat depthFormat;
	Pipeline pipeline;
	Pipeline pipelineNoTexture;
	Pipeline pipelineShadow;

	CommandBuffer commandBuffer;

	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;
	uint32_t currentFrame = 0;

	RenderPass renderpass;
	RenderPass shadowpass;
	VkDescriptorPool descriptorPool{ VK_NULL_HANDLE };

	VkDescriptorSetLayout descriptorSetLayoutPhong;
	VkDescriptorSetLayout descriptorSetLayoutTexture;
	VkDescriptorSetLayout descriptorSetLayoutMVP;

	std::vector<VkDescriptorSet> descriptorSetObj1;
	std::vector<VkDescriptorSet> descriptorSetObj2;
	std::vector<VkDescriptorSet> descriptorSetFloor;
	std::vector<VkDescriptorSet> descriptorSetTexture;
	std::vector<VkDescriptorSet> descriptorSetObj1FromLight;
	std::vector<VkDescriptorSet> descriptorSetObj2FromLight;
	std::vector<VkDescriptorSet> descriptorSetFloorFromLight;

	std::vector<Buffer> lightInfoBuffer;
	std::vector<Buffer> obj1Buffer;
	std::vector<Buffer> obj2Buffer;
	std::vector<Buffer> floorBuffer;
	std::vector<Buffer> obj1FromLightBuffer;
	std::vector<Buffer> obj2FromLightBuffer;
	std::vector<Buffer> floorFromLightBuffer;
	MVP obj1;
	MVP obj2;
	MVP floor;
	MVP obj1FromLight;
	MVP obj2FromLight;
	MVP floorFromLight;
	LightInfo lightInfo;
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

