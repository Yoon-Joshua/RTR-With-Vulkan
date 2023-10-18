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

	/// \brief generate shadow map
	void create(Context* context_, VkFormat depthFormat);

	/// \brief generate G-Buffer
	void create(Context* context_, VkFormat colorFormat, VkFormat depthFormat);

	void destroy();

	void createFramebuffers(SwapChain& swapchain, Image& color, Image& depth, uint32_t width, uint32_t height);
	void createFramebuffers(Image& depth, uint32_t width, uint32_t height);

	// \brief create framebuffers of G-Buffer
	void createFramebuffers(SwapChain& swapchain,Image& worldPos, Image& normal, Image& depth, uint32_t width, uint32_t height);

	// deprecated
	void createFramebuffers(Image& resolvedColor, Image& color, Image& depth, uint32_t width, uint32_t height);
	void destroyFramebuffers();

	VkFramebuffer getFramebuffer(uint32_t index);

	VkRenderPass handle{ VK_NULL_HANDLE };
protected:
	Context* context{ nullptr };
	std::vector<VkFramebuffer> framebuffers;
};