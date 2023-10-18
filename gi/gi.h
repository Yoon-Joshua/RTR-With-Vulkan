#pragma once

#include <GLFW/glfw3.h>
#include <vector>
#include <array>
#include <memory>
#include "applicationbase.h"
#include "image.h"
#include "common.h"
#include "command_buffer.h"
#include "renderpass.h"
#include "scene.h"
#include "pipeline.h"
#include "texture.h"
#include "arcball.h"

/// @note Swapchain and Surface are necessary.
class GIApplication :public Application {
public:
	GIApplication(GlfwContext* glfwContext_):Application(glfwContext_) {}
	void prepare();
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
	void createDescriptorSetLayoutNoTexture();
	void createDescriptorSetLayoutShadow();
	void createDescriptorSet();
	void createDescriptorSetNoTexture();
	void createDescriptorSetShadow();
	/************************************************/
	Scene scene;
	Image depthImage;
	Image colorImage;
	Texture texture;
	Texture shadowMap;
	Texture normalMap;
	Texture worldPosMap;

	MVP mvp;
	std::vector<Buffer> mvpBuffer;
	MVP mvpLight;
	std::vector<Buffer> mvpLightBuffer;
	Light light;
	std::vector<Buffer> lightBuffer;

	Pipeline pipeline;
	Pipeline pipelineNoTexture;
	Pipeline pipelineShadow;

	VkDescriptorSetLayout descriptorSetLayout{ VK_NULL_HANDLE };
	VkDescriptorSetLayout descriptorSetLayoutNoTexture{ VK_NULL_HANDLE };
	VkDescriptorSetLayout descriptorSetLayoutShadow{ VK_NULL_HANDLE };

	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;
	uint32_t currentFrame = 0;

	RenderPass gbufferpass;
	RenderPass shadowpass;
	RenderPass renderpass;
	VkDescriptorPool descriptorPool{ VK_NULL_HANDLE };
	std::vector<VkDescriptorSet> descriptorSet;
	std::vector<VkDescriptorSet> descriptorSetNoTexture;
	std::vector<VkDescriptorSet> descriptorSetShadow;
};