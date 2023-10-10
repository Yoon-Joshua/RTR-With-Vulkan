#include "buffer.h"
#include "command_buffer.h"

void Buffer::create(Context* context_, VkDeviceSize size_, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) {
    context = context_;

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size_;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(context->device, &bufferInfo, nullptr, &handle) != VK_SUCCESS) {
        throw std::runtime_error("failed to create buffer!");
    }
    this->size = size_;

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(context->device, handle, &memRequirements);
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = context->findMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(context->device, &allocInfo, nullptr, &memory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate buffer memory!");
    }

    vkBindBufferMemory(context->device, handle, memory, 0);
}

void Buffer::destroy() {
    if (mapped) {
        vkUnmapMemory(context->device, memory);
    }
    vkDestroyBuffer(context->device, handle, nullptr);
    vkFreeMemory(context->device, memory, nullptr);
}

void Buffer::upload(void* data, bool staging) {
    if (staging) {
        Buffer stagingBuffer;
        stagingBuffer.create(context, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
        void* p;
        vkMapMemory(context->device, stagingBuffer.memory, 0, size, 0, &p);
        memcpy(p, data, (size_t)size);
        vkUnmapMemory(context->device, stagingBuffer.memory);
        stagingBuffer.copyTo(this, size);
        stagingBuffer.destroy();
    }
    else {
        if (!mapped) {
            vkMapMemory(context->device, memory, 0, size, 0, &map);
            mapped = true;
        }
        memcpy(map, data, (size_t)size);
    }
}

void Buffer::copyTo(Buffer* dst, VkDeviceSize size) {
    CommandBuffer commandBuffer = CommandBuffer::beginSingleTimeCommands(context);

    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0;  // Optional
    copyRegion.dstOffset = 0;  // Optional
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer.handles[0], handle, dst->handle, 1, &copyRegion);

    CommandBuffer::endSingleTimeCommands(commandBuffer);
}
