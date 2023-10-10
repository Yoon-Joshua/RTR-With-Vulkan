#pragma once
#include <vector>
#include <array>
#include <memory>
#include <vulkan/vulkan.h>
#include "context.h"
#include "swapchain.h"
#include "image.h"

class Framebuffer {
public:
	VkFramebuffer handle;
};

class RenderPass
{
public:
	void create(Context* context_, VkFormat colorFormat, VkFormat depthFormat, VkSampleCountFlagBits sampleCount);
	void destroy();

	void createFramebuffers(SwapChain& swapchain, Image& color, Image& depth, uint32_t width, uint32_t height);
	void createFramebuffers(Image& resolvedColor, Image& color, Image& depth, uint32_t width, uint32_t height);
	void destroyFramebuffers();

	VkFramebuffer getFramebuffer(uint32_t index);

	VkRenderPass handle{ VK_NULL_HANDLE };
protected:
	Context* context{ nullptr };
	std::vector<VkFramebuffer> framebuffers;
};