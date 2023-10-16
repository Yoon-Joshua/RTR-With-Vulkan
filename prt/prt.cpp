#include "prt.h"
#include "common.h"
#include <chrono>

const std::string TEXTURE_PATH_ = "E:/model/marry/MC003_Kozakura_Mari.png";

//std::array<const char*, 6> CUBE_MAP_PATH = {
//	"E:/games202/Assignment2/prt/scenes/cubemap/Indoor/posx.jpg",	// left
//	"E:/games202/Assignment2/prt/scenes/cubemap/Indoor/negx.jpg",	// right
//	"E:/games202/Assignment2/prt/scenes/cubemap/Indoor/posy.jpg",	// top
//	"E:/games202/Assignment2/prt/scenes/cubemap/Indoor/negy.jpg",	// bottom
//	"E:/games202/Assignment2/prt/scenes/cubemap/Indoor/posz.jpg",	// front
//	"E:/games202/Assignment2/prt/scenes/cubemap/Indoor/negz.jpg",	// back
//};

//std::array<const char*, 6> CUBE_MAP_PATH = {
//	"E:/games202/Assignment2/prt/scenes/cubemap/CornellBox/posx.jpg",	// left
//	"E:/games202/Assignment2/prt/scenes/cubemap/CornellBox/negx.jpg",	// right
//	"E:/games202/Assignment2/prt/scenes/cubemap/CornellBox/posy.jpg",	// top
//	"E:/games202/Assignment2/prt/scenes/cubemap/CornellBox/negy.jpg",	// bottom
//	"E:/games202/Assignment2/prt/scenes/cubemap/CornellBox/posz.jpg",	// front
//	"E:/games202/Assignment2/prt/scenes/cubemap/CornellBox/negz.jpg",	// back
//};

//std::array<const char*, 6> CUBE_MAP_PATH = {
//	"E:/games202/Assignment2/prt/scenes/cubemap/Skybox/posx.jpg",	// left
//	"E:/games202/Assignment2/prt/scenes/cubemap/Skybox/negx.jpg",	// right
//	"E:/games202/Assignment2/prt/scenes/cubemap/Skybox/posy.jpg",	// top
//	"E:/games202/Assignment2/prt/scenes/cubemap/Skybox/negy.jpg",	// bottom
//	"E:/games202/Assignment2/prt/scenes/cubemap/Skybox/posz.jpg",	// front
//	"E:/games202/Assignment2/prt/scenes/cubemap/Skybox/negz.jpg",	// back
//};

std::array<const char*, 6> CUBE_MAP_PATH = {
	"E:/games202/Assignment2/prt/scenes/cubemap/GraceCathedral/posx.jpg",	// left
	"E:/games202/Assignment2/prt/scenes/cubemap/GraceCathedral/negx.jpg",	// right
	"E:/games202/Assignment2/prt/scenes/cubemap/GraceCathedral/posy.jpg",	// top
	"E:/games202/Assignment2/prt/scenes/cubemap/GraceCathedral/negy.jpg",	// bottom
	"E:/games202/Assignment2/prt/scenes/cubemap/GraceCathedral/posz.jpg",	// front
	"E:/games202/Assignment2/prt/scenes/cubemap/GraceCathedral/negz.jpg",	// back
};

void PRTApplication::init(GlfwContext* glfwContext_) {
	glfwContext = glfwContext_;
	context.create(glfwContext);

	swapchain.create(&context, glfwContext);

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
}

void PRTApplication::prepare() {
	depthImage.create(&context, swapchain.width(), swapchain.height(),
		1, context.msaaSamples, depthFormat, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_DEPTH_BIT);

	colorImage.create(&context, swapchain.width(), swapchain.height(),
		1, context.msaaSamples, swapchain.getFormat(), VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_COLOR_BIT);

	depthImage.transitionImageLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

	renderpass.create(&context, swapchain.getFormat(), depthFormat, context.msaaSamples);
	renderpass.createFramebuffers(swapchain, colorImage, depthImage, swapchain.width(), swapchain.height());

	createDescriptorSetLayout();
	createDescriptorSetLayoutSkyBox();
	std::vector<VkDescriptorSetLayout> layouts(1, descriptorSetLayout);
	pipeline.createPipelinePRT(&context, layouts, renderpass);
	layouts = { descriptorSetLayoutSkyBox };
	pipelineSkyBox.createPipelineSkyBox(&context, layouts, renderpass);

	mvpBuffer.resize(MAX_FRAMES_IN_FLIGHT);
	lightPrtBuffer.resize(MAX_FRAMES_IN_FLIGHT);
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		mvpBuffer[i].create(&context, sizeof(MVP), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		lightPrtBuffer[i].create(&context, sizeof(LightPRT), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	}
	texture.create(&context, TEXTURE_PATH_.c_str());
	skybox.create(&context, CUBE_MAP_PATH);

	createDescriptorPool();
	createDescriptorSet();
	createDescriptorSetSkyBox();

	createSyncObjects();
	scene.loadModelPRT(&context);
}

void PRTApplication::mainLoop(Timer& timer) {
	commandBuffer.create(&context, MAX_FRAMES_IN_FLIGHT);

	timer.reset();

	while (!glfwWindowShouldClose(glfwContext->window)) {
		timer.tick();
		glfwPollEvents();
		drawFrame();
	}
	vkDeviceWaitIdle(context.getDevice());

	commandBuffer.destroy();
}

void PRTApplication::cleanup() {
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		vkDestroySemaphore(context.getDevice(), imageAvailableSemaphores[i], nullptr);
		vkDestroySemaphore(context.getDevice(), renderFinishedSemaphores[i], nullptr);
		vkDestroyFence(context.getDevice(), inFlightFences[i], nullptr);
		mvpBuffer[i].destroy();
		lightPrtBuffer[i].destroy();
	}
	renderpass.destroyFramebuffers();
	colorImage.destroy();
	depthImage.destroy();
	texture.destroy();
	skybox.destroy();
	renderpass.destroy();
	swapchain.destroy();
	pipeline.destroy();
	pipelineSkyBox.destroy();
	vkDestroyDescriptorSetLayout(context.device, descriptorSetLayout, nullptr);
	vkDestroyDescriptorSetLayout(context.device, descriptorSetLayoutSkyBox, nullptr);
	vkDestroyDescriptorPool(context.device, descriptorPool, nullptr);

	scene.cleanup();
	context.destroy();
}

void PRTApplication::record(uint32_t imageIndex) {

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
	renderPassInfo.renderPass = renderpass.handle;
	renderPassInfo.framebuffer = renderpass.getFramebuffer(imageIndex);
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent.width = swapchain.width();
	renderPassInfo.renderArea.extent.height = swapchain.height();
	std::array<VkClearValue, 2> clearValues{};
	clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
	clearValues[1].depthStencil = { 1.0f, 0 };
	renderPassInfo.clearValueCount = 2;
	renderPassInfo.pClearValues = clearValues.data();
	vkCmdBeginRenderPass(cb, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.handle);
	VkBuffer vertexBuffers[] = { scene.vertexBuffer.handle };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(cb, 0, 1, vertexBuffers, offsets);
	vkCmdBindIndexBuffer(cb, scene.indexBuffer.handle, 0, VK_INDEX_TYPE_UINT32);
	vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.layout, 0, 1, &descriptorSet[currentFrame], 0, nullptr);

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = w;
	viewport.height = h;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(cb, 0, 1, &viewport);
	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent.width = w;
	scissor.extent.height = h;
	vkCmdSetScissor(cb, 0, 1, &scissor);
	vkCmdDrawIndexed(cb, scene.meshes[0].indices.size() - 36, 1, 0, 0, 0);

	vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineSkyBox.handle);
	vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineSkyBox.layout, 0, 1, &descriptorSetSkyBox[currentFrame], 0, nullptr);
	vkCmdDrawIndexed(cb, 36, 1, scene.meshes[0].indices.size() - 36, 0, 0);

	vkCmdEndRenderPass(cb);

	if (vkEndCommandBuffer(cb) != VK_SUCCESS) {
		throw std::runtime_error("failed to record command buffer!");
	}
}

void PRTApplication::drawFrame() {
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

void PRTApplication::createSyncObjects() {
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

void PRTApplication::resizeWindow() {
	int width = 0, height = 0;
	while (width == 0 || height == 0) {
		glfwGetFramebufferSize(glfwContext->window, &width, &height);
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

void PRTApplication::updateUniformObjects(size_t index) {
	mvp.model = glm::mat4(1.0);
	mvp.view = glm::lookAt(glm::vec3(0.f, 2.f, 8.f), glm::vec3(0.f, 2.f, 0.f), glm::vec3(0.f, 1.f, 0.f));
	//mvp.view = glm::lookAt(glm::vec3(0.f, 8.f, 22.f), glm::vec3(0.f, 1.f, 1.f), glm::vec3(0.f, 1.f, 0.f));
	mvp.proj = glm::perspective(glm::radians(45.f), (float)swapchain.width() / (float)swapchain.height(), 1.f, 1000.f);
	mvp.proj[1][1] *= -1;
	mvpBuffer[index].upload(&mvp, false);

	lightPrt.r = scene.lightCoef[0];
	lightPrt.g = scene.lightCoef[1];
	lightPrt.b = scene.lightCoef[2];
	lightPrtBuffer[index].upload(&lightPrt, false);
}

void PRTApplication::createDescriptorPool() {
	std::array<VkDescriptorPoolSize, 2> poolSizes{};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = static_cast<uint32_t>(4 * MAX_FRAMES_IN_FLIGHT);
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = static_cast<uint32_t>(2 * MAX_FRAMES_IN_FLIGHT);

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = static_cast<uint32_t>(2 * MAX_FRAMES_IN_FLIGHT);

	if (vkCreateDescriptorPool(context.device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor pool!");
	}
}

void PRTApplication::createDescriptorSetLayout() {
	std::array<VkDescriptorSetLayoutBinding, 3> bindings;
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
	bindings[2].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	bindings[2].pImmutableSamplers = nullptr;  // Optional

	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();
	if (vkCreateDescriptorSetLayout(context.device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor set layout!");
	}
}

void PRTApplication::createDescriptorSetLayoutSkyBox() {
	std::array<VkDescriptorSetLayoutBinding, 2> bindings;
	bindings[0].binding = 0;
	bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	bindings[0].descriptorCount = 1;
	bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	bindings[0].pImmutableSamplers = nullptr;  // Optional
	bindings[1].binding = 1;
	bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	bindings[1].descriptorCount = 1;
	bindings[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	bindings[1].pImmutableSamplers = nullptr;  // Optional

	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();
	if (vkCreateDescriptorSetLayout(context.device, &layoutInfo, nullptr, &descriptorSetLayoutSkyBox) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor set layout!");
	}
}

void PRTApplication::createDescriptorSet() {
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
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = mvpBuffer[i].handle;
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(MVP);

		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = texture.image.view;
		imageInfo.sampler = texture.sampler;

		VkDescriptorBufferInfo lightPrtInfo{};
		lightPrtInfo.buffer = lightPrtBuffer[i].handle;
		lightPrtInfo.offset = 0;
		lightPrtInfo.range = sizeof(LightPRT);

		std::array<VkWriteDescriptorSet, 3> descriptorWrites{};
		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = descriptorSet[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &bufferInfo;
		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = descriptorSet[i];
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pImageInfo = &imageInfo;
		descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[2].dstSet = descriptorSet[i];
		descriptorWrites[2].dstBinding = 2;
		descriptorWrites[2].dstArrayElement = 0;
		descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[2].descriptorCount = 1;
		descriptorWrites[2].pBufferInfo = &lightPrtInfo;

		vkUpdateDescriptorSets(context.device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}
}

void PRTApplication::createDescriptorSetSkyBox() {
	std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayoutSkyBox);
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
	allocInfo.pSetLayouts = layouts.data();

	descriptorSetSkyBox.resize(MAX_FRAMES_IN_FLIGHT);
	if (vkAllocateDescriptorSets(context.device, &allocInfo, descriptorSetSkyBox.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate descriptor sets!");
	}

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = skybox.image.view;
		imageInfo.sampler = skybox.sampler;
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = mvpBuffer[i].handle;
		bufferInfo.offset = 0;
		bufferInfo.range = VK_WHOLE_SIZE;

		std::array<VkWriteDescriptorSet, 2> descriptorWrites{};
		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = descriptorSetSkyBox[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pImageInfo = &imageInfo;
		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = descriptorSetSkyBox[i];
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pBufferInfo = &bufferInfo;

		vkUpdateDescriptorSets(context.device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}
}


