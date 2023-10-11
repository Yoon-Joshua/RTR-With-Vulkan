#include "Application.h"
#include "common.h"
std::string TEXTURE_PATH = "E:/model/marry/MC003_Kozakura_Mari.png";

unsigned count = 0;

void Application::init() {
	context.create();
	arcball.init(context.window);

	swapchain.create(&context);

	camera.setAspect((float)swapchain.width() / (float)swapchain.height());

	std::vector<VkFormat> candidates = { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT };
	VkFormatFeatureFlagBits features = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
	bool flag = false;
	for (VkFormat format : candidates) {
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(context.getPhysicalDevice(), format, &props);
		if ((props.optimalTilingFeatures & features) == features) {
			depthFormat = format;
			flag = true;
			break;
		}
	}
	if (!flag) throw std::runtime_error("failed to find supported format!");

	depthImage.create(&context, swapchain.width(), swapchain.height(),
		1, context.msaaSamples, depthFormat, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_DEPTH_BIT);

	colorImage.create(&context, swapchain.width(), swapchain.height(),
		1, context.msaaSamples, swapchain.getFormat(), VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, 
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_COLOR_BIT);

	depthImage.transitionImageLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

	shadowDepthImage.create(&context, SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT,
		1, context.msaaSamples, depthFormat, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_DEPTH_BIT);
	shadowColorImage.create(&context, SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT,
		1, context.msaaSamples, swapchain.getFormat(), VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
	shadowmap.create(&context, SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT);
	
	renderpass.create(&context, swapchain.getFormat(), depthFormat, context.msaaSamples);
	renderpass.createFramebuffers(swapchain, colorImage, depthImage, swapchain.width(), swapchain.height());

	shadowpass.create(&context, swapchain.getFormat(), depthFormat, context.msaaSamples);
	shadowpass.createFramebuffers(shadowmap.image, shadowColorImage, shadowDepthImage, SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT);
	//shadowpass.createFramebuffers(swapchain, shadowColorImage, shadowDepthImage, SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT);
	
	cameraBuffer.resize(MAX_FRAMES_IN_FLIGHT);
	lightBuffer.resize(MAX_FRAMES_IN_FLIGHT);
	lightInfoBuffer.resize(MAX_FRAMES_IN_FLIGHT);
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		cameraBuffer[i].create(&context, sizeof(MVP), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		lightBuffer[i].create(&context, sizeof(MVP), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		lightInfoBuffer[i].create(&context, sizeof(lightInfo), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
	}
	texture.create(&context, TEXTURE_PATH.c_str());

	createDescriptorPool();
	createDescriptorSetLayoutPhong();
	createDescriptorSetLayoutTexture();
	createDescriptorSetLayoutMVP();
	createDescriptorSetsPhong();
	createDescriptorSetsTexture();
	createDescriptorSetsMVP();

	std::vector<VkDescriptorSetLayout> temp;
	temp = { descriptorSetLayoutPhong, descriptorSetLayoutTexture };
	pipeline.createPipeline(&context, temp, renderpass);
	temp = { descriptorSetLayoutPhong };
	pipelineNoTexture.createPipelineNoTexture(&context, temp, renderpass);
	temp = { descriptorSetLayoutMVP };
	pipelineShadow.createShadowPipeline(&context, temp, shadowpass);

	createSyncObjects();
	scene.loadModel(&context);
}

void Application::mainLoop(Timer& timer) {
	commandBuffer.create(&context, MAX_FRAMES_IN_FLIGHT);

	timer.reset();

	while (!glfwWindowShouldClose(context.getWindow())) {
		timer.tick();
		glfwPollEvents();
		drawFrame();
	}
	vkDeviceWaitIdle(context.getDevice());

	commandBuffer.destroy();
}

void Application::cleanup() {
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		vkDestroySemaphore(context.getDevice(), imageAvailableSemaphores[i], nullptr);
		vkDestroySemaphore(context.getDevice(), renderFinishedSemaphores[i], nullptr);
		vkDestroyFence(context.getDevice(), inFlightFences[i], nullptr);

		cameraBuffer[i].destroy();
		lightBuffer[i].destroy();
		lightInfoBuffer[i].destroy();
	}
	renderpass.destroyFramebuffers();
	shadowpass.destroyFramebuffers();
	colorImage.destroy();
	depthImage.destroy();
	texture.destroy();
	shadowColorImage.destroy();
	shadowDepthImage.destroy();
	shadowmap.destroy();
	renderpass.destroy();
	shadowpass.destroy();
	swapchain.destroy();
	pipeline.destroy();
	pipelineNoTexture.destroy();
	pipelineShadow.destroy();

	vkDestroyDescriptorSetLayout(context.device, descriptorSetLayoutPhong, nullptr);
	vkDestroyDescriptorSetLayout(context.device, descriptorSetLayoutTexture, nullptr);
	vkDestroyDescriptorSetLayout(context.device, descriptorSetLayoutMVP, nullptr);
	vkDestroyDescriptorPool(context.device, descriptorPool, nullptr);

	scene.cleanup();
	context.destroy();
}

void Application::record(uint32_t imageIndex) {

	VkCommandBuffer& cb = commandBuffer.handles[currentFrame];

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	if (vkBeginCommandBuffer(cb, &beginInfo) != VK_SUCCESS) {
		throw std::runtime_error("failed to begin recording command buffer!");
	}

	// shadow render pass
	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = shadowpass.handle;
	renderPassInfo.framebuffer = shadowpass.getFramebuffer(0);
	//renderPassInfo.framebuffer = shadowpass.getFramebuffer(imageIndex);
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent.width = SHADOW_MAP_WIDTH;
	renderPassInfo.renderArea.extent.height = SHADOW_MAP_HEIGHT;
	std::array<VkClearValue, 2> clearValues{};
	clearValues[0].color = { {1.0f, 1.0f, 1.0f, 1.0f} };
	clearValues[1].depthStencil = { 1.0f, 0 };
	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();
	vkCmdBeginRenderPass(cb, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineShadow.handle);
	VkBuffer vertexBuffers[] = { scene.vertexBuffer.handle };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(cb, 0, 1, vertexBuffers, offsets);
	vkCmdBindIndexBuffer(cb, scene.indexBuffer.handle, 0, VK_INDEX_TYPE_UINT32);
	vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineShadow.layout, 0, 1, &descriptorSetMVP[currentFrame], 0, nullptr);

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
	vkCmdDrawIndexed(cb, scene.meshes[0].indices.size(), 1, 0, 0, 0);
	vkCmdEndRenderPass(cb);

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.image = shadowmap.image.handle;
	barrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

	renderPassInfo.renderPass = renderpass.handle;
	renderPassInfo.framebuffer = renderpass.getFramebuffer(imageIndex);
	renderPassInfo.renderArea.extent.width = swapchain.width();
	renderPassInfo.renderArea.extent.height = swapchain.height();
	vkCmdBeginRenderPass(cb, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.handle);
	std::array<VkDescriptorSet, 2> temp = { descriptorSetPhong[currentFrame],descriptorSetTexture[currentFrame] };
	vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.layout, 0, 2, temp.data(), 0, nullptr);
	viewport.width = swapchain.width();
	viewport.height = swapchain.height();
	vkCmdSetViewport(cb, 0, 1, &viewport);
	scissor.extent.width = swapchain.width();
	scissor.extent.height = swapchain.height();
	vkCmdSetScissor(cb, 0, 1, &scissor);
	vkCmdDrawIndexed(cb, scene.meshes[0].indices.size() - 6, 1, 0, 0, 0);

	vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineNoTexture.handle);
	vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineNoTexture.layout, 0, 1, &descriptorSetPhong[currentFrame], 0, nullptr);
	vkCmdDrawIndexed(cb, 6, 1, scene.meshes[0].indices.size() - 6, 0, 0);
	vkCmdEndRenderPass(cb);

	if (vkEndCommandBuffer(cb) != VK_SUCCESS) {
		throw std::runtime_error("failed to record command buffer!");
	}
}

void Application::drawFrame() {
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

	updateUniformBuffers(currentFrame);

	std::vector<RenderPass> passes = { shadowpass,renderpass };
	std::vector<Pipeline> pipelines = { pipelineShadow, pipeline, pipelineNoTexture };
	std::vector<VkDescriptorSet> sets = { descriptorSetMVP[currentFrame], descriptorSetPhong[currentFrame],
										descriptorSetTexture[currentFrame],descriptorSetPhong[currentFrame] };

	//commandBuffer.record(currentFrame, imageIndex, shadowpass, pipelines, sets, swapchain.getExtent(), scene);
	record(imageIndex);

	commandBuffer.submit(currentFrame, imageAvailableSemaphores[currentFrame], renderFinishedSemaphores[currentFrame], inFlightFences[currentFrame]);

	result = swapchain.present(imageIndex, renderFinishedSemaphores[currentFrame]);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || context.framebufferResized) {
		context.framebufferResized = false;
		resizeWindow();
	}
	else if (result != VK_SUCCESS) {
		throw std::runtime_error("failed to present swap chain image!");
	}

	currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Application::createSyncObjects() {
	imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		if (vkCreateSemaphore(context.getDevice(), &semaphoreInfo, nullptr,	&imageAvailableSemaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(context.getDevice(), &semaphoreInfo, nullptr,	&renderFinishedSemaphores[i]) != VK_SUCCESS ||
			vkCreateFence(context.getDevice(), &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to create synchronization objects for a frame!");
		}
	}
}

void Application::resizeWindow() {
	int width = 0, height = 0;
	while (width == 0 || height == 0) {
		glfwGetFramebufferSize(context.window, &width, &height);
		glfwWaitEvents();
	}
	vkDeviceWaitIdle(context.device);
	swapchain.recreateSwapChain();
	uint32_t w = swapchain.width();
	uint32_t h = swapchain.height();
	camera.setAspect((float)w / (float)h);
	depthImage.resize(w, h);
	colorImage.resize(w, h);
	renderpass.destroyFramebuffers();
	renderpass.createFramebuffers(swapchain, colorImage, depthImage, w, h);
}

void Application::updateUniformBuffers(size_t index) {
	camera.setFovy(ArcBall::scroll_factor);
	//cameraMVP.model = glm::mat4(1.0);
	cameraMVP.model = arcball.get();
	cameraMVP.view = camera.lookAt();
	cameraMVP.proj = camera.perspective();
	cameraBuffer[index].upload(&cameraMVP, false);

	lightInfo.pos = glm::vec4(8.0f, 8.0f, -8.0f, 1.0f);
	lightInfo.intensity = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	lightInfoBuffer[index].upload(&lightInfo, false);

	float w = swapchain.width();
	float h = swapchain.height();
	lightMVP.model = glm::mat4(1.0);
	lightMVP.view = glm::lookAt(lightInfo.pos, glm::vec3(0.0, -1.0, 0.0), glm::vec3(0.0, 1.0, 0.0));
	lightMVP.proj = glm::perspective(glm::radians(45.0f), w / h, 7.0f, 20.0f);
	lightMVP.proj[1][1] *= -1;
	lightBuffer[index].upload(&lightMVP, false);
}

void Application::createDescriptorPool() {
	std::array<VkDescriptorPoolSize, 2> poolSizes{};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = static_cast<uint32_t>(6 * MAX_FRAMES_IN_FLIGHT);
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ;
	poolSizes[1].descriptorCount = static_cast<uint32_t>(4 * MAX_FRAMES_IN_FLIGHT);

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = static_cast<uint32_t>(3 * MAX_FRAMES_IN_FLIGHT);

	if (vkCreateDescriptorPool(context.device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor pool!");
	}
}

void Application::createDescriptorSetLayoutPhong() {
	std::array<VkDescriptorSetLayoutBinding, 4> bindings;
	bindings[0].binding = 0;
	bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	bindings[0].descriptorCount = 1;
	bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	bindings[0].pImmutableSamplers = nullptr;  // Optional

	bindings[1].binding = 1;
	bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	bindings[1].descriptorCount = 1;
	bindings[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	bindings[1].pImmutableSamplers = nullptr;  // Optional

	bindings[2].binding = 2;
	bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	bindings[2].descriptorCount = 1;
	bindings[2].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	bindings[2].pImmutableSamplers = nullptr;  // Optional

	bindings[3].binding = 3;
	bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	bindings[3].descriptorCount = 1;
	bindings[3].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	bindings[3].pImmutableSamplers = nullptr;  // Optional

	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();
	if (vkCreateDescriptorSetLayout(context.device, &layoutInfo, nullptr, &descriptorSetLayoutPhong) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor set layout!");
	}
}

void Application::createDescriptorSetLayoutTexture() {
	std::array<VkDescriptorSetLayoutBinding, 1> bindings;
	bindings[0].binding = 0;
	bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	bindings[0].descriptorCount = 1;
	bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	bindings[0].pImmutableSamplers = nullptr;  // Optional
	//bindings[1].binding = 1;
	//bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	//bindings[1].descriptorCount = 1;
	//bindings[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	//bindings[1].pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();
	if (vkCreateDescriptorSetLayout(context.device, &layoutInfo, nullptr, &descriptorSetLayoutTexture) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor set layout!");
	}
}

void Application::createDescriptorSetLayoutMVP() {
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
	if (vkCreateDescriptorSetLayout(context.device, &layoutInfo, nullptr, &descriptorSetLayoutMVP) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor set layout!");
	}
}

void Application::createDescriptorSetsPhong() {
	std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayoutPhong);
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
	allocInfo.pSetLayouts = layouts.data();

	descriptorSetPhong.resize(MAX_FRAMES_IN_FLIGHT);
	if (vkAllocateDescriptorSets(context.device, &allocInfo, descriptorSetPhong.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate descriptor sets!");
	}

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		std::array<VkDescriptorBufferInfo, 4> bufferInfo;
		bufferInfo[0].buffer = cameraBuffer[i].handle;
		bufferInfo[0].offset = 0;
		bufferInfo[0].range = sizeof(MVP);
		bufferInfo[1].buffer = lightBuffer[i].handle;
		bufferInfo[1].offset = 0;
		bufferInfo[1].range = sizeof(MVP);
		bufferInfo[2].buffer = lightInfoBuffer[i].handle;
		bufferInfo[2].offset = 0;
		bufferInfo[2].range = sizeof(LightInfo);

		VkDescriptorImageInfo imageInfo{};
		imageInfo.sampler = shadowmap.sampler;
		imageInfo.imageView = shadowmap.image.view;
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		std::array<VkWriteDescriptorSet, 4> descriptorWrites{};
		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = descriptorSetPhong[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &bufferInfo[0];

		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = descriptorSetPhong[i];
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pBufferInfo = &bufferInfo[1];

		descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[2].dstSet = descriptorSetPhong[i];
		descriptorWrites[2].dstBinding = 2;
		descriptorWrites[2].dstArrayElement = 0;
		descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[2].descriptorCount = 1;
		descriptorWrites[2].pBufferInfo = &bufferInfo[2];

		descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[3].dstSet = descriptorSetPhong[i];
		descriptorWrites[3].dstBinding = 3;
		descriptorWrites[3].dstArrayElement = 0;
		descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[3].descriptorCount = 1;
		descriptorWrites[3].pImageInfo = &imageInfo;

		vkUpdateDescriptorSets(context.device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}
}

void Application::createDescriptorSetsTexture() {
	std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayoutTexture);
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
	allocInfo.pSetLayouts = layouts.data();

	descriptorSetTexture.resize(MAX_FRAMES_IN_FLIGHT);
	if (vkAllocateDescriptorSets(context.device, &allocInfo, descriptorSetTexture.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate descriptor sets!");
	}

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		std::array<VkDescriptorImageInfo,1> imageInfo{};
		imageInfo[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo[0].imageView = texture.image.view;
		imageInfo[0].sampler = texture.sampler;
		//imageInfo[1].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		//imageInfo[1].imageView = shadowmap.image.view;
		//imageInfo[1].sampler = shadowmap.sampler;

		std::array<VkWriteDescriptorSet, 1> descriptorWrites{};
		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = descriptorSetTexture[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pImageInfo = &imageInfo[0];
		//descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		//descriptorWrites[1].dstSet = descriptorSetTexture[i];
		//descriptorWrites[1].dstBinding = 1;
		//descriptorWrites[1].dstArrayElement = 0;
		//descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		//descriptorWrites[1].descriptorCount = 1;
		//descriptorWrites[1].pImageInfo = &imageInfo[1];
		vkUpdateDescriptorSets(context.device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}
}

void Application::createDescriptorSetsMVP() {
	std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayoutMVP);
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
	allocInfo.pSetLayouts = layouts.data();

	descriptorSetMVP.resize(MAX_FRAMES_IN_FLIGHT);
	if (vkAllocateDescriptorSets(context.device, &allocInfo, descriptorSetMVP.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate descriptor sets!");
	}

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = lightBuffer[i].handle;
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(MVP);

		std::array<VkWriteDescriptorSet, 1> descriptorWrites{};
		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = descriptorSetMVP[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &bufferInfo;

		vkUpdateDescriptorSets(context.device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}
}
