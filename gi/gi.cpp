#include "gi.h"
#include "common.h"
#include <chrono>

const std::string TEXTURE_PATH = "E:/games202/homework3/assets/cube/checker.png";

void GIApplication::prepare() {
	/*depthImage.create(&context, swapchain.width(), swapchain.height(),
		1, context.msaaSamples, depthFormat, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_DEPTH_BIT);*/
	depthImage.create(&context, swapchain.width(), swapchain.height(),
		1, VK_SAMPLE_COUNT_1_BIT, depthFormat, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_DEPTH_BIT);

	colorImage.create(&context, swapchain.width(), swapchain.height(),
		1, context.msaaSamples, swapchain.getFormat(), VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_COLOR_BIT);

	depthImage.transitionImageLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

	shadowMap.create(&context, SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT, VK_FORMAT_D16_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_DEPTH_BIT);
	normalMap.create(&context, swapchain.width(), swapchain.height(), VK_FORMAT_B8G8R8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
	worldPosMap.create(&context, swapchain.width(), swapchain.height(), VK_FORMAT_B8G8R8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_COLOR_BIT);

	texture.create(&context, TEXTURE_PATH);

	/*renderpass.create(&context, swapchain.getFormat(), depthFormat, context.msaaSamples);
	renderpass.createFramebuffers(swapchain, colorImage, depthImage, swapchain.width(), swapchain.height());*/
	gbufferpass.create(&context, swapchain.getFormat(), depthFormat);
	gbufferpass.createFramebuffers(swapchain, worldPosMap.image, normalMap.image, depthImage, swapchain.width(), swapchain.height());
	shadowpass.create(&context, VK_FORMAT_D16_UNORM);
	shadowpass.createFramebuffers(shadowMap.image, SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT);

	createDescriptorSetLayout();
	createDescriptorSetLayoutNoTexture();
	createDescriptorSetLayoutShadow();
	std::vector<VkDescriptorSetLayout> layouts(1, descriptorSetLayout);
	//pipeline.createPipeline(&context, layouts, renderpass);
	pipeline.createPipelineGBuffer(&context, layouts, gbufferpass);
	layouts = { descriptorSetLayoutNoTexture };
	//pipelineNoTexture.createPipelineNoTexture(&context, layouts, renderpass);
	layouts = { descriptorSetLayoutShadow };
	pipelineShadow.createShadowPipeline(&context, layouts, shadowpass);

	mvpBuffer.resize(MAX_FRAMES_IN_FLIGHT);
	lightBuffer.resize(MAX_FRAMES_IN_FLIGHT);
	mvpLightBuffer.resize(MAX_FRAMES_IN_FLIGHT);
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		mvpBuffer[i].create(&context, sizeof(MVP), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		lightBuffer[i].create(&context, sizeof(Light), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		mvpLightBuffer[i].create(&context, sizeof(MVP), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	}

	createDescriptorPool();
	createDescriptorSet();
	createDescriptorSetNoTexture();
	createDescriptorSetShadow();

	createSyncObjects();
	scene.loadGLFT(&context);
}

void GIApplication::cleanup() {
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		vkDestroySemaphore(context.getDevice(), imageAvailableSemaphores[i], nullptr);
		vkDestroySemaphore(context.getDevice(), renderFinishedSemaphores[i], nullptr);
		vkDestroyFence(context.getDevice(), inFlightFences[i], nullptr);
		mvpBuffer[i].destroy();
		lightBuffer[i].destroy();
		mvpLightBuffer[i].destroy();
	}
	gbufferpass.destroyFramebuffers();
	shadowpass.destroyFramebuffers();
	colorImage.destroy();
	depthImage.destroy();
	shadowMap.destroy();
	normalMap.destroy();
	worldPosMap.destroy();

	texture.destroy();
	gbufferpass.destroy();
	shadowpass.destroy();
	pipeline.destroy();
	//pipelineNoTexture.destroy();
	pipelineShadow.destroy();
	vkDestroyDescriptorSetLayout(context.device, descriptorSetLayout, nullptr);
	vkDestroyDescriptorSetLayout(context.device, descriptorSetLayoutNoTexture, nullptr);
	vkDestroyDescriptorSetLayout(context.device, descriptorSetLayoutShadow, nullptr);
	vkDestroyDescriptorPool(context.device, descriptorPool, nullptr);

	scene.cleanup();
}

void GIApplication::record(uint32_t imageIndex) {

	VkCommandBuffer& cb = commandBuffer.handles[currentFrame];

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	if (vkBeginCommandBuffer(cb, &beginInfo) != VK_SUCCESS) {
		throw std::runtime_error("failed to begin recording command buffer!");
	}
	uint32_t w = swapchain.width();
	uint32_t h = swapchain.height();

	// shadow render pass
	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = shadowpass.handle;
	renderPassInfo.framebuffer = shadowpass.getFramebuffer(0);
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent.width = SHADOW_MAP_WIDTH;
	renderPassInfo.renderArea.extent.height = SHADOW_MAP_HEIGHT;
	VkClearValue clearDepth{};
	clearDepth.depthStencil = { 1.0f,0 };
	renderPassInfo.clearValueCount = 1;
	renderPassInfo.pClearValues = &clearDepth;
	vkCmdBeginRenderPass(cb, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	VkBuffer vertexBuffers[] = { scene.vertexBuffer.handle };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(cb, 0, 1, vertexBuffers, offsets);
	vkCmdBindIndexBuffer(cb, scene.indexBuffer.handle, 0, VK_INDEX_TYPE_UINT32);
	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = SHADOW_MAP_WIDTH;
	viewport.height = SHADOW_MAP_HEIGHT;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(cb, 0, 1, &viewport);
	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent.width = SHADOW_MAP_WIDTH;
	scissor.extent.height = SHADOW_MAP_HEIGHT;
	vkCmdSetScissor(cb, 0, 1, &scissor);
	uint32_t indexCount = 0;
	uint32_t firstIndex = 0;
	int32_t vertexOffset = 0;
	vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineShadow.handle);
	vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineShadow.layout, 0, 1, &descriptorSetShadow[currentFrame], 0, nullptr);
	indexCount = scene.meshes[0].indices.size();
	firstIndex = scene.meshes[0].indexOffset;
	vkCmdDrawIndexed(cb, indexCount, 1, firstIndex, 0, 0);
	indexCount = scene.meshes[1].indices.size();
	firstIndex = scene.meshes[1].indexOffset;
	vertexOffset = scene.meshes[1].vertexOffset;
	vkCmdDrawIndexed(cb, indexCount, 1, firstIndex, vertexOffset, 0);
	vkCmdEndRenderPass(cb);

	renderPassInfo.renderPass = gbufferpass.handle;
	renderPassInfo.framebuffer = gbufferpass.getFramebuffer(imageIndex);
	renderPassInfo.renderArea.extent.width = w;
	renderPassInfo.renderArea.extent.height = h;
	std::array<VkClearValue, 4> clearValues{};
	clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
	clearValues[1].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
	clearValues[2].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
	clearValues[3].depthStencil = { 1.0f, 0 };
	renderPassInfo.clearValueCount = clearValues.size();
	renderPassInfo.pClearValues = clearValues.data();
	vkCmdBeginRenderPass(cb, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	viewport.width = w;
	viewport.height = h;
	vkCmdSetViewport(cb, 0, 1, &viewport);	
	scissor.extent.width = w;
	scissor.extent.height = h;
	vkCmdSetScissor(cb, 0, 1, &scissor);

	indexCount = scene.meshes[0].indices.size();
	firstIndex = scene.meshes[0].indexOffset;
	vertexOffset = scene.meshes[0].vertexOffset;
	vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.handle);
	vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.layout, 0, 1, &descriptorSet[currentFrame], 0, nullptr);
	vkCmdDrawIndexed(cb, indexCount, 1, firstIndex, vertexOffset, 0);
	indexCount = scene.meshes[1].indices.size();
	firstIndex = scene.meshes[1].indexOffset;
	vertexOffset = scene.meshes[1].vertexOffset;
	vkCmdDrawIndexed(cb, indexCount, 1, firstIndex, vertexOffset, 0);
	vkCmdEndRenderPass(cb);

	if (vkEndCommandBuffer(cb) != VK_SUCCESS) {
		throw std::runtime_error("failed to record command buffer!");
	}
}

void GIApplication::drawFrame() {
	vkWaitForFences(context.getDevice(), 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

	uint32_t imageIndex;
	VkResult result = swapchain.acquireNextImage(imageAvailableSemaphores[currentFrame], imageIndex);
	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		resizeWindow();
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		throw std::runtime_error("failed to acquire swap chain image!");
	}

	vkResetFences(context.getDevice(), 1, &inFlightFences[currentFrame]);

	commandBuffer.reset(currentFrame);

	updateUniformObjects(currentFrame);

	record(imageIndex);

	commandBuffer.submit(currentFrame, imageAvailableSemaphores[currentFrame], renderFinishedSemaphores[currentFrame], inFlightFences[currentFrame]);

	result = swapchain.present(imageIndex, renderFinishedSemaphores[currentFrame]);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || GlfwContext::framebufferResized) {
		GlfwContext::framebufferResized = false;
		resizeWindow();
	}
	else if (result != VK_SUCCESS) {
		throw std::runtime_error("failed to present swap chain image!");
	}

	currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void GIApplication::createSyncObjects() {
	imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		if (vkCreateSemaphore(context.getDevice(), &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(context.getDevice(), &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
			vkCreateFence(context.getDevice(), &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to create synchronization objects for a frame!");
		}
	}
}

void GIApplication::resizeWindow() {
	int width = 0, height = 0;
	while (width == 0 || height == 0) {
		glfwGetFramebufferSize(glfwContext->window, &width, &height);
		glfwWaitEvents();
	}
	vkDeviceWaitIdle(context.device);
	swapchain.recreateSwapChain();
	uint32_t w = swapchain.width();
	uint32_t h = swapchain.height();
	depthImage.resize(w, h);
	colorImage.resize(w, h);

	worldPosMap.image.resize(w, h);
	normalMap.image.resize(w, h);

	gbufferpass.destroyFramebuffers();
	//renderpass.createFramebuffers(swapchain, colorImage, depthImage, w, h);
	gbufferpass.createFramebuffers(swapchain, worldPosMap.image, normalMap.image, depthImage, w, h);
}

void GIApplication::updateUniformObjects(size_t index) {
	mvp.model = glm::mat4(1.0);
	mvp.view = glm::lookAt(glm::vec3(10.f, 15.f, 10.f), glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, 1.f, 0.f));
	mvp.proj = glm::perspective(glm::radians(65.f), (float)swapchain.width() / (float)swapchain.height(), 1.f, 1000.f);
	mvp.proj[1][1] *= -1;
	mvpBuffer[index].upload(&mvp, false);

	light.pos = glm::vec3(-2.0f, 4.0f, 2.0f);
	light.intensity = glm::vec3(1.0f, 1.0f, 1.0f);
	lightBuffer[index].upload(&light, false);

	mvpLight.model = glm::mat4(1.0);
	mvpLight.view = glm::lookAt(light.pos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	mvpLight.proj = glm::ortho(-60.0f, 60.0f, -30.0f, 30.f, 0.1f, 100.0f);
	mvpLightBuffer[index].upload(&mvpLight, false);
}

void GIApplication::createDescriptorPool() {
	std::array<VkDescriptorPoolSize, 2> poolSizes{};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = static_cast<uint32_t>(7 * MAX_FRAMES_IN_FLIGHT);
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = static_cast<uint32_t>(3 * MAX_FRAMES_IN_FLIGHT);

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = static_cast<uint32_t>(3 * MAX_FRAMES_IN_FLIGHT);

	if (vkCreateDescriptorPool(context.device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor pool!");
	}
}

void GIApplication::createDescriptorSetLayout() {
	std::array<VkDescriptorSetLayoutBinding, 5> bindings;
	bindings[0].binding = 0;
	bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	bindings[0].descriptorCount = 1;
	bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	bindings[0].pImmutableSamplers = nullptr;  // Optional
	bindings[1].binding = 1;
	bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	bindings[1].descriptorCount = 1;
	bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	bindings[1].pImmutableSamplers = nullptr;  // Optional
	bindings[2].binding = 2;
	bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	bindings[2].descriptorCount = 1;
	bindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT;
	bindings[2].pImmutableSamplers = nullptr;
	bindings[3].binding = 3;
	bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	bindings[3].descriptorCount = 1;
	bindings[3].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	bindings[3].pImmutableSamplers = nullptr;  // Optional
	bindings[4].binding = 4;
	bindings[4].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	bindings[4].descriptorCount = 1;
	bindings[4].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	bindings[4].pImmutableSamplers = nullptr;  // Optional

	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();
	if (vkCreateDescriptorSetLayout(context.device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor set layout!");
	}
}

void GIApplication::createDescriptorSetLayoutNoTexture() {
	std::array<VkDescriptorSetLayoutBinding, 4> bindings;
	bindings[0].binding = 0;
	bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	bindings[0].descriptorCount = 1;
	bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	bindings[0].pImmutableSamplers = nullptr;  // Optional
	bindings[1].binding = 1;
	bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	bindings[1].descriptorCount = 1;
	bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT;
	bindings[1].pImmutableSamplers = nullptr;
	bindings[2].binding = 2;
	bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	bindings[2].descriptorCount = 1;
	bindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	bindings[2].pImmutableSamplers = nullptr;  // Optional
	bindings[3].binding = 3;
	bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	bindings[3].descriptorCount = 1;
	bindings[3].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	bindings[3].pImmutableSamplers = nullptr;  // Optional

	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();
	if (vkCreateDescriptorSetLayout(context.device, &layoutInfo, nullptr, &descriptorSetLayoutNoTexture) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor set layout!");
	}
}

void GIApplication::createDescriptorSetLayoutShadow() {
	std::array<VkDescriptorSetLayoutBinding, 1> bindings;
	bindings[0].binding = 0;
	bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	bindings[0].descriptorCount = 1;
	bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	bindings[0].pImmutableSamplers = nullptr;  // Optional

	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();
	if (vkCreateDescriptorSetLayout(context.device, &layoutInfo, nullptr, &descriptorSetLayoutShadow) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor set layout!");
	}
}

void GIApplication::createDescriptorSet() {
	std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
	allocInfo.pSetLayouts = layouts.data();

	descriptorSet.resize(MAX_FRAMES_IN_FLIGHT);
	if (vkAllocateDescriptorSets(context.device, &allocInfo, descriptorSet.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate descriptor sets!");
	}

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		VkDescriptorBufferInfo bufferInfo[3] = {};
		bufferInfo[0].buffer = mvpBuffer[i].handle;
		bufferInfo[0].offset = 0;
		bufferInfo[0].range = sizeof(MVP);

		VkDescriptorImageInfo imageInfo[2] = {};
		imageInfo[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo[0].imageView = texture.image.view;
		imageInfo[0].sampler = texture.sampler;

		bufferInfo[1].buffer = lightBuffer[i].handle;
		bufferInfo[1].offset = 0;
		bufferInfo[1].range = sizeof(Light);

		imageInfo[1].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo[1].imageView = shadowMap.image.view;
		imageInfo[1].sampler = shadowMap.sampler;

		bufferInfo[2].buffer = mvpLightBuffer[i].handle;
		bufferInfo[2].offset = 0;
		bufferInfo[2].range = sizeof(MVP);

		std::array<VkWriteDescriptorSet, 5> descriptorWrites{};
		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = descriptorSet[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &bufferInfo[0];
		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = descriptorSet[i];
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pImageInfo = &imageInfo[0];
		descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[2].dstSet = descriptorSet[i];
		descriptorWrites[2].dstBinding = 2;
		descriptorWrites[2].dstArrayElement = 0;
		descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[2].descriptorCount = 1;
		descriptorWrites[2].pBufferInfo = &bufferInfo[1];
		descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[3].dstSet = descriptorSet[i];
		descriptorWrites[3].dstBinding = 3;
		descriptorWrites[3].dstArrayElement = 0;
		descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[3].descriptorCount = 1;
		descriptorWrites[3].pImageInfo = &imageInfo[1];
		descriptorWrites[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[4].dstSet = descriptorSet[i];
		descriptorWrites[4].dstBinding = 4;
		descriptorWrites[4].dstArrayElement = 0;
		descriptorWrites[4].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[4].descriptorCount = 1;
		descriptorWrites[4].pBufferInfo = &bufferInfo[2];

		vkUpdateDescriptorSets(context.device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}
}

void GIApplication::createDescriptorSetNoTexture() {
	std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayoutNoTexture);
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
	allocInfo.pSetLayouts = layouts.data();

	descriptorSetNoTexture.resize(MAX_FRAMES_IN_FLIGHT);
	if (vkAllocateDescriptorSets(context.device, &allocInfo, descriptorSetNoTexture.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate descriptor sets!");
	}

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		VkDescriptorBufferInfo bufferInfo[3] = {};
		bufferInfo[0].buffer = mvpBuffer[i].handle;
		bufferInfo[0].offset = 0;
		bufferInfo[0].range = sizeof(MVP);

		bufferInfo[1].buffer = lightBuffer[i].handle;
		bufferInfo[1].offset = 0;
		bufferInfo[1].range = sizeof(Light);

		VkDescriptorImageInfo imageInfo[1] = {};
		imageInfo[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo[0].imageView = shadowMap.image.view;
		imageInfo[0].sampler = shadowMap.sampler;

		bufferInfo[2].buffer = mvpLightBuffer[i].handle;
		bufferInfo[2].offset = 0;
		bufferInfo[2].range = sizeof(MVP);

		std::array<VkWriteDescriptorSet, 4> descriptorWrites{};
		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = descriptorSetNoTexture[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &bufferInfo[0];
		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = descriptorSetNoTexture[i];
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pBufferInfo = &bufferInfo[1];
		descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[2].dstSet = descriptorSetNoTexture[i];
		descriptorWrites[2].dstBinding = 2;
		descriptorWrites[2].dstArrayElement = 0;
		descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[2].descriptorCount = 1;
		descriptorWrites[2].pImageInfo = &imageInfo[0];
		descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[3].dstSet = descriptorSetNoTexture[i];
		descriptorWrites[3].dstBinding = 3;
		descriptorWrites[3].dstArrayElement = 0;
		descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[3].descriptorCount = 1;
		descriptorWrites[3].pBufferInfo = &bufferInfo[2];

		vkUpdateDescriptorSets(context.device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}
}

void GIApplication::createDescriptorSetShadow() {
	std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayoutShadow);
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
	allocInfo.pSetLayouts = layouts.data();

	descriptorSetShadow.resize(MAX_FRAMES_IN_FLIGHT);
	if (vkAllocateDescriptorSets(context.device, &allocInfo, descriptorSetShadow.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate descriptor sets!");
	}

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = mvpLightBuffer[i].handle;
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(MVP);

		std::array<VkWriteDescriptorSet, 1> descriptorWrites{};
		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = descriptorSetShadow[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &bufferInfo;

		vkUpdateDescriptorSets(context.device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}
}

