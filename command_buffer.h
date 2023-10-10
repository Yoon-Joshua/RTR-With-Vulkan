#pragma once
#include <vulkan/vulkan.h>
#include "context.h"
#include "scene.h"
#include "Pipeline.h"

class RenderPass;

class CommandBuffer
{
public:
	static CommandBuffer beginSingleTimeCommands(Context* context);

	static void endSingleTimeCommands(CommandBuffer& cb);

	void create(Context* context, uint32_t num);

	void destroy();

	void record(uint32_t, uint32_t, RenderPass pass, std::vector<Pipeline>&, std::vector<VkDescriptorSet>&, VkExtent2D, Scene&);

	void reset(uint32_t bufferIndex);

	void submit(uint32_t bufferIndex, VkSemaphore waitSemaphore, VkSemaphore signalSemaphore, VkFence fence);

	std::vector<VkCommandBuffer> handles;

	// ÒÀÀµ pool --> queue --> device   
	VkCommandPool pool;
	VkQueue queue;
	VkDevice device;
};

