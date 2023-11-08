#pragma once
#include <vulkan/vulkan.h>
#include "context.h"

class Image
{
public:
	void create(Context* context_, uint32_t width, uint32_t height, uint32_t mipLevels,
		VkSampleCountFlagBits numSamples, VkFormat format,
		VkImageUsageFlags usage, VkImageAspectFlagBits aspectFlags,
		bool isCube = false);

	void destroy();

	void transitionImageLayout(VkImageLayout oldLayout, VkImageLayout newLayout);

	void copyToImage(Image&);

	void resize(uint32_t width, uint32_t height);
	
	Context* context;
	VkImage handle{ VK_NULL_HANDLE };
	VkImageView readView{ VK_NULL_HANDLE };
	VkImageView writeView{ VK_NULL_HANDLE };
	VmaAllocation allocation;

	VkFormat format;
	uint32_t layers{ 1 };
	uint32_t mipLevels{ 1 };
	uint32_t width;
	uint32_t height;
	VkSampleCountFlagBits numSamples;
	VkImageUsageFlags usage;
	VkImageAspectFlagBits aspectFlags;
};

