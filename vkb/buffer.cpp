#include "buffer.h"

#include "command_buffer.h"

void Buffer::create(Context* cnt, VkBufferUsageFlags usage, VkDeviceSize size,
                    bool hostVisible) {
  context = cnt;
  this->size = size;

  VkBufferCreateInfo bufferInfo{};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = size;
  bufferInfo.usage = usage;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VmaAllocationCreateInfo allocInfo{};
  allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
  if (hostVisible)
    allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

  if (vmaCreateBuffer(context->allocator, &bufferInfo, &allocInfo, &handle,
                      &allocation, &allocationInfo) != VK_SUCCESS) {
    throw std::runtime_error("failed to create buffer using VMA!");
  }
}

void Buffer::destroy() {
  vmaDestroyBuffer(context->allocator, handle, allocation);
}

void Buffer::upload(void* data, bool staging) {
  if (staging) {
    Buffer stagingBuffer;
    stagingBuffer.create(context, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, size, true);
    void* pData;
    vmaMapMemory(context->allocator,stagingBuffer.allocation,&pData);
    memcpy(pData, data, (size_t)size);
    vmaUnmapMemory(context->allocator,stagingBuffer.allocation);
    stagingBuffer.copyTo(this, size);
    stagingBuffer.destroy();
  } else {
    void* pData;
    vmaMapMemory(context->allocator,this->allocation,&pData);
    memcpy(pData, data, (size_t)size);
    vmaUnmapMemory(context->allocator,allocation);
  }
}

void Buffer::copyTo(Buffer* dst, VkDeviceSize size) {
  CommandBuffer commandBuffer = CommandBuffer::beginSingleTimeCommands(context);

  VkBufferCopy copyRegion{};
  copyRegion.srcOffset = 0;  // Optional
  copyRegion.dstOffset = 0;  // Optional
  copyRegion.size = size;
  vkCmdCopyBuffer(commandBuffer.handle, handle, dst->handle, 1, &copyRegion);

  CommandBuffer::endSingleTimeCommands(commandBuffer);
}
