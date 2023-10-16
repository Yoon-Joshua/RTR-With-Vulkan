#pragma once

#include <string>
#include "image.h"

class Texture {
public:
	Image image;
	VkSampler sampler;
	Context* context;

	void generateMipmaps();

	void create(Context* context_, uint32_t texWidth, uint32_t texHeight, VkFormat format, VkImageTiling tiling,
		VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImageAspectFlagBits aspectFlags);

	void create(Context* context_, std::string path);

	/// \brief create cube map
	void create(Context* context_, std::array<const char*, 6> path);

	void destroy();
};
