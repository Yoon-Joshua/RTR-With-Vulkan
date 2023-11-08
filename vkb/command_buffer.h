#pragma once
#include <vulkan/vulkan.h>

#include "context.h"

class RenderPass;

class CommandBuffer {
 public:
  static CommandBuffer beginSingleTimeCommands(Context* context);

  static void endSingleTimeCommands(CommandBuffer& cb);

  static std::vector<CommandBuffer> create(Context* context, uint32_t num);

  // void create(Context* context, uint32_t num);
  void create(Context* context) {
    pool = context->commandPool;
    queue = context->graphicsQueue;
    device = context->device;

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = context->commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    if (vkAllocateCommandBuffers(device, &allocInfo, &handle) != VK_SUCCESS) {
      throw std::runtime_error("failed to allocate command buffers!");
    }
  }

  void destroy();

  void reset(uint32_t bufferIndex);

  void submit(uint32_t bufferIndex, VkSemaphore waitSemaphore,
              VkSemaphore signalSemaphore, VkFence fence);

  std::vector<VkCommandBuffer> handles;
  VkCommandBuffer handle{VK_NULL_HANDLE};

  // pool --> queue --> device
  VkCommandPool pool;
  VkQueue queue;
  VkDevice device;
};
