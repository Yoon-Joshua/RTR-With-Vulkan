#include "command_buffer.h"
#include "common.h"
#include <array>

CommandBuffer CommandBuffer::beginSingleTimeCommands(Context* context) {

	CommandBuffer cb;
	cb.create(context, 1);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(cb.handles[0], &beginInfo);
	return cb;
}

void CommandBuffer::endSingleTimeCommands(CommandBuffer& cb) {
	vkEndCommandBuffer(cb.handles[0]);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cb.handles[0];

	vkQueueSubmit(cb.queue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(cb.queue);
}

void CommandBuffer::create(Context* context, uint32_t num) {
	handles.resize(num, VK_NULL_HANDLE);
	pool = context->getCommandPool();
	queue = context->graphicsQueue;
	device = context->getDevice();

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = context->getCommandPool();
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t)handles.size();

	if (vkAllocateCommandBuffers(device, &allocInfo, handles.data()) !=
		VK_SUCCESS) {
		throw std::runtime_error("failed to allocate command buffers!");
	}
}

void CommandBuffer::destroy() {
	vkFreeCommandBuffers(device, pool, handles.size(), handles.data());
}

void CommandBuffer::record(uint32_t bufferIndex, uint32_t imageIndex, RenderPass pass, std::vector<Pipeline>& pipelines,
	std::vector<VkDescriptorSet>& sets, VkExtent2D extent, Scene& scene) {
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	if (vkBeginCommandBuffer(handles[bufferIndex], &beginInfo) != VK_SUCCESS) {
		throw std::runtime_error("failed to begin recording command buffer!");
	}

	// shadow render pass
	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = pass.handle;
	renderPassInfo.framebuffer = pass.getFramebuffer(0);
	//renderPassInfo.framebuffer = passes[0].getFramebuffer(imageIndex);
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent.width = SHADOW_MAP_WIDTH;
	renderPassInfo.renderArea.extent.height = SHADOW_MAP_HEIGHT;
	std::array<VkClearValue, 2> clearValues{};
	clearValues[0].color = { {1.0f, 1.0f, 1.0f, 1.0f} };
	clearValues[1].depthStencil = { 1.0f, 0 };
	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();
	vkCmdBeginRenderPass(handles[bufferIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);


	vkCmdBindPipeline(handles[bufferIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[0].handle);
	VkBuffer vertexBuffers[] = { scene.vertexBuffer.handle };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(handles[bufferIndex], 0, 1, vertexBuffers, offsets);
	vkCmdBindIndexBuffer(handles[bufferIndex], scene.indexBuffer.handle, 0, VK_INDEX_TYPE_UINT32);
	vkCmdBindDescriptorSets(handles[bufferIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[0].layout, 0, 1, &sets[0], 0, nullptr);

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = SHADOW_MAP_WIDTH;
	viewport.height = SHADOW_MAP_HEIGHT;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(handles[bufferIndex], 0, 1, &viewport);
	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent.width = SHADOW_MAP_WIDTH;
	scissor.extent.height = SHADOW_MAP_HEIGHT;
	vkCmdSetScissor(handles[bufferIndex], 0, 1, &scissor);
	vkCmdDrawIndexed(handles[bufferIndex], scene.meshes[0].indices.size(), 1, 0, 0, 0);

	vkCmdEndRenderPass(handles[bufferIndex]);

	// common render pass
	//renderPassInfo.renderPass = passes[1].handle;
	//renderPassInfo.framebuffer = passes[1].getFramebuffer(imageIndex);
	//renderPassInfo.renderArea.extent = extent;
	//clearValues[0].color = { {1.0f, 1.0f, 1.0f, 1.0f} };
	//vkCmdBeginRenderPass(handles[bufferIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	//vkCmdBindPipeline(handles[bufferIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[1].handle);
	//viewport.width = (float)extent.width;
	//viewport.height = (float)extent.height;
	//vkCmdSetViewport(handles[bufferIndex], 0, 1, &viewport);
	//scissor.extent = extent;
	//vkCmdSetScissor(handles[bufferIndex], 0, 1, &scissor);
	//VkBuffer vertexBuffers[] = { scene.vertexBuffer.handle };
	//VkDeviceSize offsets[] = { 0 };
	//vkCmdBindVertexBuffers(handles[bufferIndex], 0, 1, vertexBuffers, offsets);
	//vkCmdBindIndexBuffer(handles[bufferIndex], scene.indexBuffer.handle, 0, VK_INDEX_TYPE_UINT32);
	//vkCmdBindDescriptorSets(handles[bufferIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[1].layout, 0, 2, &sets[1], 0, nullptr);
	//vkCmdDrawIndexed(handles[bufferIndex], scene.meshes[0].indices.size() - 6, 1, 0, 0, 0);

	//vkCmdBindPipeline(handles[bufferIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[2].handle);
	//vkCmdBindDescriptorSets(handles[bufferIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[2].layout, 0, 1, &sets[3], 0, nullptr);
	//vkCmdDrawIndexed(handles[bufferIndex], 6, 1, scene.meshes[0].indices.size() - 6, 0, 0);

	//vkCmdEndRenderPass(handles[bufferIndex]);

	if (vkEndCommandBuffer(handles[bufferIndex]) != VK_SUCCESS) {
		throw std::runtime_error("failed to record command buffer!");
	}
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
	submitInfo.pCommandBuffers = &handles[bufferIndex];

	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &signalSemaphore;

	if (vkQueueSubmit(queue, 1, &submitInfo, fence) != VK_SUCCESS) {
		throw std::runtime_error("failed to submit draw command buffer!");
	}
}
