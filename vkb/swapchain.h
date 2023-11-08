#pragma once

#include <unordered_set>
#include <vector>
#include <limits>
#include <algorithm>
#include <stdexcept>
#include <vulkan/vulkan.h>
#include "image.h"

class SwapChain {
public:
	struct SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	void create(Context* context_, GlfwContext* glfwContext_);
	void destroy();

	VkFormat getFormat()const;
	VkExtent2D getExtent()const;
	uint32_t width()const;
	uint32_t height()const;
	size_t getNumImages()const;
	VkImageView getView(size_t index)const;

	VkResult acquireNextImage(VkSemaphore semaphore, uint32_t& imageIndex);
	VkResult present(uint32_t imageIndex, VkSemaphore waitSemaphore);
	void recreateSwapChain();

private:
	GlfwContext* glfwContext{ nullptr };
	Context* context{ nullptr };
	/***************************************************************/
	VkSwapchainKHR handle{ VK_NULL_HANDLE };
	std::vector<VkImage> images;
	std::vector<VkImageView> imageViews;
	uint32_t currentIndex{ 0 };
	VkFormat format{ VK_FORMAT_UNDEFINED };
	VkExtent2D extent{ 0,0 };

	SwapChainSupportDetails querySwapChainSupport();

	void createSwapChain(GlfwContext* );

	void createImageViews();

	void cleanupSwapChain();
};