#include "gi.h"
#include "common.h"
#include <chrono>

//const std::string TEXTURE_PATH = "E:/games202/homework3/assets/cube/checker.png";
const std::string TEXTURE_PATH = "E:/games202/homework3/assets/cave/siEoZ_2K_Albedo.jpg";

void GIApplication::prepare() {
	depthImage.create(&context, swapchain.width(), swapchain.height(),
		1, context.msaaSamples, depthFormat, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_DEPTH_BIT);

	colorImage.create(&context, swapchain.width(), swapchain.height(),
		1, context.msaaSamples, swapchain.getFormat(), VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_COLOR_BIT);

	depthImage.transitionImageLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

	shadowMap.create(&context, SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT, VK_FORMAT_D16_UNORM, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_DEPTH_BIT);
	normalMap.create(&context, swapchain.width(), swapchain.height(), VK_FORMAT_B8G8R8A8_SRGB,
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
	worldPosMap.create(&context, swapchain.width(), swapchain.height(), VK_FORMAT_B8G8R8A8_SRGB,
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
	depthMap.create(&context, swapchain.width(), swapchain.height(), depthFormat,
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_ASPECT_DEPTH_BIT, false);

	colorMap.create(&context, swapchain.width(), swapchain.height(), VK_FORMAT_B8G8R8A8_SRGB,
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_IMAGE_ASPECT_COLOR_BIT);

	texture.create(&context, TEXTURE_PATH);

	renderpass.create(&context, swapchain.getFormat(), depthFormat, context.msaaSamples);
	renderpass.createFramebuffers(swapchain, colorImage, depthImage, swapchain.width(), swapchain.height());
	gbufferpass.create(&context, swapchain.getFormat(), depthFormat);
	gbufferpass.createFramebuffers(colorMap.image, worldPosMap.image, normalMap.image, depthMap.image, swapchain.width(), swapchain.height());
	shadowpass.create(&context, VK_FORMAT_D16_UNORM);
	shadowpass.createFramebuffers(shadowMap.image, SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT);

	createDescriptorSetLayout();
	std::vector<VkDescriptorSetLayout> layouts;
	layouts = { lightSetLayout };
	pipelineShadow.createShadowPipeline(&context, layouts, shadowpass);
	layouts = { transformSetLayout, lightSetLayout, materialSetLayout};
	pipeline.createPipelineGBuffer(&context, layouts, gbufferpass);
	layouts = { transformSetLayout, lightSetLayout};
	pipelineNoTexture.createPipelineNoTexture(&context, layouts, gbufferpass);
	layouts = { transformSetLayout, lightSetLayout, gBufferSetLayout };
	pipelineSSR.createPipelineSSR(&context, layouts, renderpass);

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
	writeDescriptorSet();

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
	renderpass.destroyFramebuffers();
	gbufferpass.destroyFramebuffers();
	shadowpass.destroyFramebuffers();
	colorImage.destroy();
	depthImage.destroy();
	shadowMap.destroy();
	normalMap.destroy();
	worldPosMap.destroy();
	depthMap.destroy();
	colorMap.destroy();

	texture.destroy();
	renderpass.destroy();
	gbufferpass.destroy();
	shadowpass.destroy();
	pipeline.destroy();
	pipelineNoTexture.destroy();
	pipelineShadow.destroy();
	pipelineSSR.destroy();
	vkDestroyDescriptorSetLayout(context.device, transformSetLayout, nullptr);
	vkDestroyDescriptorSetLayout(context.device, lightSetLayout, nullptr);
	vkDestroyDescriptorSetLayout(context.device, materialSetLayout, nullptr);
	vkDestroyDescriptorSetLayout(context.device, gBufferSetLayout, nullptr);
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
	vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineShadow.layout, 0, 1, &lightSet[currentFrame], 0, nullptr);
	indexCount = static_cast<uint32_t>(scene.meshes[0].indices.size());
	firstIndex = static_cast<uint32_t>(scene.meshes[0].indexOffset);
	vkCmdDrawIndexed(cb, indexCount, 1, firstIndex, 0, 0);
	//indexCount = static_cast<uint32_t>(scene.meshes[1].indices.size());
	//firstIndex = static_cast<uint32_t>(scene.meshes[1].indexOffset);
	//vertexOffset = static_cast<uint32_t>(scene.meshes[1].vertexOffset);
	//vkCmdDrawIndexed(cb, indexCount, 1, firstIndex, vertexOffset, 0);
	vkCmdEndRenderPass(cb);

	renderPassInfo.renderPass = gbufferpass.handle;
	renderPassInfo.framebuffer = gbufferpass.getFramebuffer(0);
	renderPassInfo.renderArea.extent.width = w;
	renderPassInfo.renderArea.extent.height = h;
	std::array<VkClearValue, 4> clearValues{};
	clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
	clearValues[1].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
	clearValues[2].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
	clearValues[3].depthStencil = { 1.0f, 0 };
	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();
	vkCmdBeginRenderPass(cb, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	viewport.width = static_cast<float>(w);
	viewport.height = static_cast<float>(h);
	vkCmdSetViewport(cb, 0, 1, &viewport);	
	scissor.extent.width = w;
	scissor.extent.height = h;
	vkCmdSetScissor(cb, 0, 1, &scissor);

	indexCount = static_cast<uint32_t>(scene.meshes[0].indices.size());
	firstIndex = static_cast<uint32_t>(scene.meshes[0].indexOffset);
	vertexOffset = static_cast<uint32_t>(scene.meshes[0].vertexOffset);
	vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.handle);
	std::vector<VkDescriptorSet> sets = { transformSet[currentFrame], lightSet[currentFrame], materialSet[currentFrame] };
	vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.layout, 0, sets.size(), sets.data(), 0, nullptr);
	vkCmdDrawIndexed(cb, indexCount, 1, firstIndex, vertexOffset, 0);
	//indexCount = static_cast<uint32_t>(scene.meshes[1].indices.size());
	//firstIndex = static_cast<uint32_t>(scene.meshes[1].indexOffset);
	//vertexOffset = static_cast<uint32_t>(scene.meshes[1].vertexOffset);
	//vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineNoTexture.handle);
	//vkCmdDrawIndexed(cb, indexCount, 1, firstIndex, vertexOffset, 0);
	vkCmdEndRenderPass(cb);

	renderPassInfo.renderPass = renderpass.handle;
	renderPassInfo.framebuffer = renderpass.getFramebuffer(imageIndex);
	std::array<VkClearValue, 2> clearValuesSSR{};
	clearValuesSSR[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
	clearValuesSSR[1].depthStencil = { 1.0f, 0 };
	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValuesSSR.size());
	renderPassInfo.pClearValues = clearValuesSSR.data();
	vkCmdBeginRenderPass(cb, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineSSR.handle);
	vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineSSR.layout, 2, 1, &gBufferSet[currentFrame], 0, nullptr);

	indexCount = static_cast<uint32_t>(scene.meshes[0].indices.size());
	firstIndex = static_cast<uint32_t>(scene.meshes[0].indexOffset);
	vertexOffset = static_cast<uint32_t>(scene.meshes[0].vertexOffset);
	vkCmdDrawIndexed(cb, indexCount, 1, firstIndex, vertexOffset, 0);
	//indexCount = static_cast<uint32_t>(scene.meshes[1].indices.size());
	//firstIndex = static_cast<uint32_t>(scene.meshes[1].indexOffset);
	//vertexOffset = static_cast<uint32_t>(scene.meshes[1].vertexOffset);
	//vkCmdDrawIndexed(cb, indexCount, 1, firstIndex, vertexOffset, 0);
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
	depthMap.image.resize(w, h);
	colorMap.image.resize(w, h);
	writeDescriptorSet();

	gbufferpass.destroyFramebuffers();
	gbufferpass.createFramebuffers(colorMap.image, worldPosMap.image, normalMap.image, depthMap.image, w, h);
	renderpass.destroyFramebuffers();
	renderpass.createFramebuffers(swapchain, colorImage, depthImage, w, h);
}

void GIApplication::updateUniformObjects(size_t index) {
	glm::vec3 eye = { 4.18927f, 1.0313f, 2.07331f };
	glm::vec3 target = { 2.92191f, 0.98f, 1.55037f };
	mvp.model = glm::mat4(1.0);
	mvp.view = glm::lookAt(eye, target, glm::vec3(0.f, 1.f, 0.f));
	//mvp.view = glm::lookAt(glm::vec3(6.f, 1.f, 0.f), glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, 1.f, 0.f));
	mvp.proj = glm::perspective(glm::radians(65.f), (float)swapchain.width() / (float)swapchain.height(), 0.1f, 100.f);
	mvp.proj[1][1] *= -1;
	mvp.eye = eye;
	mvpBuffer[index].upload(&mvp, false);

	light.pos = glm::vec3(-0.45f, 5.40507f, 0.637043f);
	light.intensity = 5.0f * glm::vec3(1.0f, 1.0f, 1.0f);
	lightBuffer[index].upload(&light, false);

	mvpLight.model = glm::mat4(1.0);
	mvpLight.view = glm::lookAt(light.pos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	mvpLight.proj = glm::ortho(-60.0f, 60.0f, -30.0f, 30.f, 0.1f, 20.0f);
	mvpLightBuffer[index].upload(&mvpLight, false);
}

void GIApplication::createDescriptorPool() {
	std::array<VkDescriptorPoolSize, 2> poolSizes{};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = static_cast<uint32_t>(9 * MAX_FRAMES_IN_FLIGHT);
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = static_cast<uint32_t>(8 * MAX_FRAMES_IN_FLIGHT);

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = static_cast<uint32_t>(4 * MAX_FRAMES_IN_FLIGHT);

	if (vkCreateDescriptorPool(context.device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor pool!");
	}
}

void GIApplication::createDescriptorSetLayout() {
	std::vector<VkDescriptorSetLayoutBinding> bindings(1);
	bindings[0].binding = 0;
	bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	bindings[0].descriptorCount = 1;
	bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	bindings[0].pImmutableSamplers = nullptr;  // Optional

	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();
	if (vkCreateDescriptorSetLayout(context.device, &layoutInfo, nullptr, &transformSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor set layout!");
	}

	bindings.resize(2);
	bindings[0].binding = 0;
	bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	bindings[0].descriptorCount = 1;
	bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	bindings[0].pImmutableSamplers = nullptr;  // Optional
	bindings[1].binding = 1;
	bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	bindings[1].descriptorCount = 1;
	bindings[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	bindings[1].pImmutableSamplers = nullptr;  // Optional
	layoutInfo.bindingCount = bindings.size();
	layoutInfo.pBindings = bindings.data();
	if (vkCreateDescriptorSetLayout(context.device, &layoutInfo, nullptr, &lightSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor set layout!");
	}

	bindings.resize(1);
	bindings[0].binding = 0;
	bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	bindings[0].descriptorCount = 1;
	bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	bindings[0].pImmutableSamplers = nullptr;  // Optional
	layoutInfo.bindingCount = bindings.size();
	layoutInfo.pBindings = bindings.data();
	if (vkCreateDescriptorSetLayout(context.device, &layoutInfo, nullptr, &materialSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor set layout!");
	}

	bindings.resize(1);
	bindings[0].binding = 0;
	bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	bindings[0].descriptorCount = 4;
	bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	bindings[0].pImmutableSamplers = nullptr;  // Optional
	layoutInfo.bindingCount = bindings.size();
	layoutInfo.pBindings = bindings.data();
	if (vkCreateDescriptorSetLayout(context.device, &layoutInfo, nullptr, &gBufferSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor set layout!");
	}
}

void GIApplication::createDescriptorSet() {
	std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, transformSetLayout);
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
	allocInfo.pSetLayouts = layouts.data();
	transformSet.resize(MAX_FRAMES_IN_FLIGHT);
	if (vkAllocateDescriptorSets(context.device, &allocInfo, transformSet.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate descriptor sets!");
	}

	layouts.assign(MAX_FRAMES_IN_FLIGHT, lightSetLayout);
	allocInfo.pSetLayouts = layouts.data();
	lightSet.resize(MAX_FRAMES_IN_FLIGHT);
	if (vkAllocateDescriptorSets(context.device, &allocInfo, lightSet.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate descriptor sets!");
	}

	layouts.assign(MAX_FRAMES_IN_FLIGHT, materialSetLayout);
	allocInfo.pSetLayouts = layouts.data();
	materialSet.resize(MAX_FRAMES_IN_FLIGHT);
	if (vkAllocateDescriptorSets(context.device, &allocInfo, materialSet.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate descriptor sets!");
	}

	layouts.assign(MAX_FRAMES_IN_FLIGHT, gBufferSetLayout);
	allocInfo.pSetLayouts = layouts.data();
	gBufferSet.resize(MAX_FRAMES_IN_FLIGHT);
	if (vkAllocateDescriptorSets(context.device, &allocInfo, gBufferSet.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate descriptor sets!");
	}
}

void GIApplication::writeDescriptorSet() {
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		VkDescriptorBufferInfo bufferInfo;
		bufferInfo.buffer = mvpBuffer[i].handle;
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(MVP);
		std::array<VkWriteDescriptorSet, 1> write{};
		write[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write[0].dstSet = transformSet[i];
		write[0].dstBinding = 0;
		write[0].dstArrayElement = 0;
		write[0].descriptorCount = 1;
		write[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		write[0].pBufferInfo = &bufferInfo;
		vkUpdateDescriptorSets(context.device, static_cast<uint32_t>(write.size()), write.data(), 0, nullptr);
	}
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		std::array<VkDescriptorBufferInfo, 2> bufferInfo{};
		bufferInfo[0].buffer = mvpLightBuffer[i].handle;
		bufferInfo[0].offset = 0;
		bufferInfo[0].range = sizeof(MVP);
		bufferInfo[1].buffer = lightBuffer[i].handle;
		bufferInfo[1].offset = 0;
		bufferInfo[1].range = sizeof(Light);
		std::array<VkWriteDescriptorSet, 2> write{};
		write[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write[0].dstSet = lightSet[i];
		write[0].dstBinding = 0;
		write[0].dstArrayElement = 0;
		write[0].descriptorCount = 1;
		write[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		write[0].pBufferInfo = &bufferInfo[0];
		write[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write[1].dstSet = lightSet[i];
		write[1].dstBinding = 1;
		write[1].dstArrayElement = 0;
		write[1].descriptorCount = 1;
		write[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		write[1].pBufferInfo = &bufferInfo[1];
		vkUpdateDescriptorSets(context.device, static_cast<uint32_t>(write.size()), write.data(), 0, nullptr);
	}
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = texture.image.readView;
		imageInfo.sampler = texture.sampler;
		std::array<VkWriteDescriptorSet, 1> write{};
		write[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write[0].dstSet = materialSet[i];
		write[0].dstBinding = 0;
		write[0].dstArrayElement = 0;
		write[0].descriptorCount = 1;
		write[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		write[0].pImageInfo = &imageInfo;
		vkUpdateDescriptorSets(context.device, static_cast<uint32_t>(write.size()), write.data(), 0, nullptr);
	}
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		std::array<VkDescriptorImageInfo, 4> imageInfo{};
		imageInfo[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo[0].imageView = shadowMap.image.readView;
		imageInfo[0].sampler = shadowMap.sampler;
		imageInfo[1].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo[1].imageView = normalMap.image.readView;
		imageInfo[1].sampler = normalMap.sampler;
		imageInfo[2].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo[2].imageView = depthMap.image.readView;
		imageInfo[2].sampler = depthMap.sampler;
		imageInfo[3].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo[3].imageView = colorMap.image.readView;
		imageInfo[3].sampler = colorMap.sampler;
		std::array<VkWriteDescriptorSet, 4> write{};
		for (int j = 0; j < write.size(); ++j) {
			write[j].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			write[j].dstSet = gBufferSet[i];
			write[j].dstBinding = 0;
			write[j].dstArrayElement = j;
			write[j].descriptorCount = 1;
			write[j].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			write[j].pImageInfo = &imageInfo[j];
		}
		vkUpdateDescriptorSets(context.device, static_cast<uint32_t>(write.size()), write.data(), 0, nullptr);
	}
}
