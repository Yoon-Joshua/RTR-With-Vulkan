#pragma once

#include <string>
#include "image.h"

class Texture {
public:
	Image image;
	VkSampler sampler;
	Context* context;

	void generateMipmaps();
public:
	void create(Context* context_, uint32_t width, uint32_t height);
	void create(Context* context_, std::string path);
	void destroy();
};
