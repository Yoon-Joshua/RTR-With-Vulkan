#include "command_buffer.h"
#include "common.h"
#include <array>

CommandBuffer CommandBuffer::beginSingleTimeCommands(Context* context) {

	CommandBuffer cb;
	cb.create(context);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(cb.handle, &beginInfo);
	return cb;
}

void CommandBuffer::endSingleTimeCommands(CommandBuffer& cb) {
	vkEndCommandBuffer(cb.handle);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cb.handle;

	vkQueueSubmit(cb.queue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(cb.queue);
}

std::vector<CommandBuffer> CommandBuffer::create(Context* context, uint32_t num){
	std::vector<CommandBuffer> cb(num);
	std::vector<VkCommandBuffer> handles(num, VK_NULL_HANDLE);

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = context->commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t)handles.size();

	if (vkAllocateCommandBuffers(context->device, &allocInfo, handles.data()) !=
		VK_SUCCESS) {
		throw std::runtime_error("failed to allocate command buffers!");
	}
	for(int i=0;i<num;++i){
		cb[i].pool = context->commandPool;
		cb[i].queue=context->graphicsQueue;
		cb[i].device=context->device;
		cb[i].handle=handles[i];
	}
	return cb;
}

// void CommandBuffer::create(Context* context, uint32_t num) {
// 	handles.resize(num, VK_NULL_HANDLE);
// 	pool = context->getCommandPool();
// 	queue = context->graphicsQueue;
// 	device = context->getDevice();

// 	VkCommandBufferAllocateInfo allocInfo{};
// 	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
// 	allocInfo.commandPool = context->getCommandPool();
// 	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
// 	allocInfo.commandBufferCount = (uint32_t)handles.size();

// 	if (vkAllocateCommandBuffers(device, &allocInfo, handles.data()) !=
// 		VK_SUCCESS) {
// 		throw std::runtime_error("failed to allocate command buffers!");
// 	}
// }

void CommandBuffer::destroy() {
	vkFreeCommandBuffers(device, pool, handles.size(), handles.data());
}

void CommandBuffer::reset(uint32_t bufferIndex) {
	vkResetCommandBuffer(handles[bufferIndex], /*VkCommandBufferResetFlagBits*/ 0);
}

void CommandBuffer::submit(uint32_t bufferIndex, VkSemaphore waitSemaphore, VkSemaphore signalSemaphore, VkFence fence) {
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &waitSemaphore;
	submitInfo.pWaitDstStageMask = waitStages;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &handle;

	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &signalSemaphore;

	if (vkQueueSubmit(queue, 1, &submitInfo, fence) != VK_SUCCESS) {
		throw std::runtime_error("failed to submit draw command buffer!");
	}
}
