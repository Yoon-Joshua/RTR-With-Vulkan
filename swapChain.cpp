#include "SwapChain.h"

void SwapChain::create(Context* context_, GlfwContext* glfwContext_) {
	context = context_;
	createSwapChain(glfwContext_);
	createImageViews();
}

void SwapChain::destroy() { cleanupSwapChain(); }

VkFormat SwapChain::getFormat()const { return format; }

VkExtent2D SwapChain::getExtent()const { return extent; }

uint32_t SwapChain::width()const { return extent.width; }

uint32_t SwapChain::height()const { return extent.height; }

size_t SwapChain::getNumImages()const { return images.size(); }

VkImageView SwapChain::getView(size_t index)const { return imageViews[index]; }

VkResult SwapChain::acquireNextImage(VkSemaphore semaphore, uint32_t& imageIndex) {
	return vkAcquireNextImageKHR(context->device, handle, UINT64_MAX, semaphore, VK_NULL_HANDLE, &imageIndex);
}

VkResult SwapChain::present(uint32_t imageIndex, VkSemaphore waitSemaphore) {
	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &waitSemaphore;

	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &handle;

	presentInfo.pImageIndices = &imageIndex;

	return vkQueuePresentKHR(context->presentQueue, &presentInfo);
}

SwapChain::SwapChainSupportDetails SwapChain::querySwapChainSupport() {
	SwapChainSupportDetails details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(context->gpu, context->surface, &details.capabilities);
	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(context->gpu, context->surface, &formatCount, nullptr);

	if (formatCount != 0) {
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(context->gpu, context->surface, &formatCount, details.formats.data());
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(context->gpu, context->surface, &presentModeCount, nullptr);

	if (presentModeCount != 0) {
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(context->gpu, context->surface, &presentModeCount,
			details.presentModes.data());
	}

	return details;
}

void SwapChain::createSwapChain(GlfwContext* glfwContext) {
	auto supports = querySwapChainSupport();
	// select format
	VkSurfaceFormatKHR surfaceFormat;
	bool flag = false;
	for (const auto &availableFormat : supports.formats) {
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
			availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			surfaceFormat = availableFormat;
			flag = true;
			break;
		}
	}
	surfaceFormat = flag ? surfaceFormat : supports.formats[0];

	// select present mode
	VkPresentModeKHR presentMode;
	flag = false;
	for (const auto &availablePresentMode : supports.presentModes) {
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
			presentMode = availablePresentMode;
			flag = true;
			break;
		}
	}
	presentMode = flag ? presentMode : VK_PRESENT_MODE_FIFO_KHR;

	// select extent
	VkExtent2D extent;
	if (supports.capabilities.currentExtent.width !=
		std::numeric_limits<uint32_t>::max()) {
		extent = supports.capabilities.currentExtent;
	}
	else {
		int width, height;
		glfwGetFramebufferSize(glfwContext->window, &width, &height);
		VkExtent2D actualExtent = { static_cast<uint32_t>(width),
								   static_cast<uint32_t>(height) };
		actualExtent.width = std::clamp(actualExtent.width,
			supports.capabilities.minImageExtent.width,
			supports.capabilities.maxImageExtent.width);
		actualExtent.height = std::clamp(
			actualExtent.height, supports.capabilities.minImageExtent.height,
			supports.capabilities.maxImageExtent.height);
		extent = actualExtent;
	}

	uint32_t imageCount = supports.capabilities.minImageCount + 1;
	if (supports.capabilities.maxImageCount > 0 &&
		imageCount > supports.capabilities.maxImageCount) {
		imageCount = supports.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = context->surface;
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	createInfo.imageSharingMode =
		VK_SHARING_MODE_EXCLUSIVE;  // 显示队列和图形队列同一个
	createInfo.preTransform = supports.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;
	if (vkCreateSwapchainKHR(context->device, &createInfo, nullptr, &handle) !=
		VK_SUCCESS) {
		throw std::runtime_error("failed to create swap chain!");
	}

	vkGetSwapchainImagesKHR(context->device, handle, &imageCount, nullptr);
	images.resize(imageCount);
	vkGetSwapchainImagesKHR(context->device, handle, &imageCount, images.data());

	this->format = surfaceFormat.format;
	this->extent = extent;
}

void SwapChain::createImageViews() {
	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = format;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;
	imageViews.resize(images.size());
	for (size_t i = 0; i < images.size(); i++) {
		viewInfo.image = images[i];
		if (vkCreateImageView(context->device, &viewInfo, nullptr, &imageViews[i]) !=
			VK_SUCCESS) {
			throw std::runtime_error("failed to create image view!");
		}
	}
}

void SwapChain::cleanupSwapChain() {
	for (VkImageView v : imageViews) {
		vkDestroyImageView(context->device, v, nullptr);
	}
	vkDestroySwapchainKHR(context->device, handle, nullptr);
}

void SwapChain::recreateSwapChain() {
	cleanupSwapChain();
	createSwapChain(glfwContext);
	createImageViews();
}
