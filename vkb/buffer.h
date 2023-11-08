#pragma once
#include <vma/vk_mem_alloc.h>
#include "context.h"

class Buffer {
public:
	/// @brief create a buffer and allocate its memory using VMA
	void create(Context* cnt, VkBufferUsageFlags usage, VkDeviceSize size, bool hostVisible);

	void destroy();
	void upload(void* data, bool staging);
	void copyTo(Buffer* dst, VkDeviceSize size);

	Context* context{ nullptr };
	VkBuffer handle{ VK_NULL_HANDLE };
	VmaAllocationInfo allocationInfo;
	VmaAllocation allocation;
	VkDeviceSize size{ 0 };
	bool mapped{ false };
};

