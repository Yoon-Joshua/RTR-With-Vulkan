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
	void init();
	void mainLoop(Timer& timer);
	void cleanup();

private:
	void record(uint32_t imageIndex);
	void drawFrame();
	void createSyncObjects();
	void resizeWindow();

	void updateUniformBuffers(size_t index);

	/************************************************/
	void createDescriptorPool();

	//void createDescriptorSetLayout();
	//void createDescriptorSetLayoutNoTexture();
	void createDescriptorSetLayoutPhong();
	void createDescriptorSetLayoutTexture();
	void createDescriptorSetLayoutMVP();

	//void createDescriptorSets();
	//void createDescriptorSetsNoTexture();
	void createDescriptorSetsPhong();
	void createDescriptorSetsTexture();
	void createDescriptorSetsMVP();
	/************************************************/

	Context context;
	Camera camera;
	ArcBall arcball;
	Scene scene;
	SwapChain swapchain;
	Image depthImage;
	Image colorImage;
	Texture texture;
	Image shadowDepthImage;
	Image shadowColorImage;
	Texture shadowmap;

	VkFormat depthFormat;
	Pipeline pipeline;
	Pipeline pipelineNoTexture;
	Pipeline pipelineShadow;

	CommandBuffer commandBuffer;
	//uint32_t MAX_FRAMES_IN_FLIGHT{ 1 };

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

	std::vector<VkDescriptorSet> descriptorSetPhong;
	std::vector<VkDescriptorSet> descriptorSetTexture;
	std::vector<VkDescriptorSet> descriptorSetMVP;

	std::vector<Buffer> cameraBuffer;
	std::vector<Buffer> lightBuffer;
	std::vector<Buffer> lightInfoBuffer;
	MVP cameraMVP;
	MVP lightMVP;
	LightInfo lightInfo;
};

/*
* flight frame
* 
* command buffer
* semaphore
* fence
* descriptor set
*/

