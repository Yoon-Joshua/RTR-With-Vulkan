#pragma once
#include <vulkan/vulkan.h>
#include "context.h"

class Buffer {
public:
	void create(Context* context_, VkDeviceSize size_, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
	void destroy();
	void upload(void* data, bool staging);
	void copyTo(Buffer* dst, VkDeviceSize size);

	Context* context{ nullptr };
	VkBuffer handle{ VK_NULL_HANDLE };
	VkDeviceMemory memory{ VK_NULL_HANDLE };
	VkDeviceSize size{ 0 };
	bool mapped{ false };
	void* map;
};

