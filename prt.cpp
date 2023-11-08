#include "prt.h"

#include <chrono>

#include "vkb/common.h"

std::array<std::string, 4> cubemapName = {"Indoor", "CornellBox", "Skybox",
                                          "GraceCathedral"};

PrtApplication::PrtApplication(GlfwContext* glfwCnt) : Application(glfwCnt) {}

void PrtApplication::prepare() {
  depthImage.create(&context, swapchain.width(), swapchain.height(), 1,
                    context.msaaSamples, depthFormat,
                    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                    VK_IMAGE_ASPECT_DEPTH_BIT);

  colorImage.create(&context, swapchain.width(), swapchain.height(), 1,
                    context.msaaSamples, swapchain.getFormat(),
                    VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT |
                        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                    VK_IMAGE_ASPECT_COLOR_BIT);

  depthImage.transitionImageLayout(
      VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

  std::array<std::string, 6> cubemapPath;
  cubemapPath[0] =
      std::string("asset/cubemap/") + cubemapName[0] + std::string("/posx.jpg");
  cubemapPath[1] =
      std::string("asset/cubemap/") + cubemapName[0] + std::string("/negx.jpg");
  cubemapPath[2] =
      std::string("asset/cubemap/") + cubemapName[0] + std::string("/posy.jpg");
  cubemapPath[3] =
      std::string("asset/cubemap/") + cubemapName[0] + std::string("/negy.jpg");
  cubemapPath[4] =
      std::string("asset/cubemap/") + cubemapName[0] + std::string("/posz.jpg");
  cubemapPath[5] =
      std::string("asset/cubemap/") + cubemapName[0] + std::string("/negz.jpg");

  cubemap.create(&context, cubemapPath);

  renderpass.create(&context, swapchain.getFormat(), depthFormat,
                    context.msaaSamples);
  renderpass.createFramebuffers(swapchain, colorImage, depthImage,
                                swapchain.width(), swapchain.height());

  createSetLayoutModel();
  createSetLayoutObject();
  createSetLayoutCubemap();
  std::vector<VkDescriptorSetLayout> layouts;
  layouts = {setLayoutModel, setLayoutObject};
  objectPipeline.createPipelinePrt(&context, layouts, renderpass);
  layouts = {setLayoutModel, setLayoutCubemap};
  PipelineCreateInfo info{};
  info.bindingDescription = VertexPrt::getBindingDescription();
  auto temp = VertexPrt::getAttributeDescriptions();
  for (auto t : temp) info.attributeDescriptions.push_back(t);
  info.fs = "shader/prt/cubemap.frag.spv";
  info.vs = "shader/prt/cubemap.vert.spv";
  info.renderPass = renderpass.handle;
  info.sampleCount = context.msaaSamples;
  info.subpass = 0;
  info.setLayouts = {setLayoutModel, setLayoutCubemap};
  info.cull = VK_CULL_MODE_FRONT_BIT;
  cubemapPipeline.create(&context, info);
  createDescriptorPool();
  scene.loadModelPrt("asset/bunny/bunny.obj");
  scene.loadModelPrt("asset/cube/cube.obj");

  scene.createGeometryBuffer(&context);
  createRenderItems();

  createFrameResource();

  std::fstream input("asset/cubemap/Skybox/light.txt", std::ios::in);
  input >> lightCoef.r[0][0] >> lightCoef.r[0][1] >> lightCoef.r[0][2] >>
      lightCoef.r[1][0] >> lightCoef.r[1][1] >> lightCoef.r[1][2] >>
      lightCoef.r[2][0] >> lightCoef.r[2][1] >> lightCoef.r[2][2];
  input >> lightCoef.g[0][0] >> lightCoef.g[0][1] >> lightCoef.g[0][2] >>
      lightCoef.g[1][0] >> lightCoef.g[1][1] >> lightCoef.g[1][2] >>
      lightCoef.g[2][0] >> lightCoef.g[2][1] >> lightCoef.g[2][2];
  input >> lightCoef.b[0][0] >> lightCoef.b[0][1] >> lightCoef.b[0][2] >>
      lightCoef.b[1][0] >> lightCoef.b[1][1] >> lightCoef.b[1][2] >>
      lightCoef.b[2][0] >> lightCoef.b[2][1] >> lightCoef.b[2][2];
  input.close();
}

void PrtApplication::cleanup() {
  destroyFrameResource();
  destroyRenderItems();
  renderpass.destroyFramebuffers();
  colorImage.destroy();
  depthImage.destroy();
  cubemap.destroy();
  renderpass.destroy();
  objectPipeline.destroy();
  cubemapPipeline.destroy();
  vkDestroyDescriptorSetLayout(context.device, setLayoutObject, nullptr);
  vkDestroyDescriptorSetLayout(context.device, setLayoutCubemap, nullptr);
  vkDestroyDescriptorSetLayout(context.device, setLayoutModel, nullptr);
  vkDestroyDescriptorPool(context.device, descriptorPool, nullptr);

  scene.cleanup();
}

void PrtApplication::record(uint32_t imageIndex) {
  VkCommandBuffer& cb = frameResource[currentFrame].commandBuffer;

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

  if (vkBeginCommandBuffer(cb, &beginInfo) != VK_SUCCESS) {
    throw std::runtime_error("failed to begin recording command buffer!");
  }
  uint32_t w = swapchain.width();
  uint32_t h = swapchain.height();

  VkRenderPassBeginInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = renderpass.handle;
  renderPassInfo.framebuffer = renderpass.getFramebuffer(imageIndex);
  renderPassInfo.renderArea.offset = {0, 0};
  renderPassInfo.renderArea.extent.width = swapchain.width();
  renderPassInfo.renderArea.extent.height = swapchain.height();
  std::array<VkClearValue, 2> clearValues{};
  clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
  clearValues[1].depthStencil = {1.0f, 0};
  renderPassInfo.clearValueCount = 2;
  renderPassInfo.pClearValues = clearValues.data();
  vkCmdBeginRenderPass(cb, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

  vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, objectPipeline.handle);
  VkBuffer vertexBuffers[] = {scene.vertexBuffer.handle};
  VkDeviceSize offsets[] = {0};
  vkCmdBindVertexBuffers(cb, 0, 1, vertexBuffers, offsets);
  vkCmdBindIndexBuffer(cb, scene.indexBuffer.handle, 0, VK_INDEX_TYPE_UINT32);
  std::array<VkDescriptorSet, 2> sets = {renderItem[0].modelSet,
                                         frameResource[currentFrame].objectSet};
  vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          objectPipeline.layout, 0, sets.size(), sets.data(), 0,
                          nullptr);

  VkViewport viewport{};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = w;
  viewport.height = h;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  vkCmdSetViewport(cb, 0, 1, &viewport);
  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent.width = w;
  scissor.extent.height = h;
  vkCmdSetScissor(cb, 0, 1, &scissor);
  vkCmdDrawIndexed(cb, renderItem[0].indexCount, 1, renderItem[0].indexOffset,
                   renderItem[0].vertexOffset, 0);

  vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    cubemapPipeline.handle);
  sets = {renderItem[1].modelSet, frameResource[currentFrame].cubemapSet};
  vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          cubemapPipeline.layout, 0, sets.size(), sets.data(),
                          0, nullptr);
  vkCmdDrawIndexed(cb, renderItem[1].indexCount, 1, renderItem[1].indexOffset,
                   renderItem[1].vertexOffset, 0);

  vkCmdEndRenderPass(cb);

  if (vkEndCommandBuffer(cb) != VK_SUCCESS) {
    throw std::runtime_error("failed to record command buffer!");
  }
}

void PrtApplication::drawFrame() {
  vkWaitForFences(context.getDevice(), 1,
                  &frameResource[currentFrame].inFlightFence, VK_TRUE,
                  UINT64_MAX);

  uint32_t imageIndex;
  VkResult result = swapchain.acquireNextImage(
      frameResource[currentFrame].imageAvailableSemaphore, imageIndex);
  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    resizeWindow();
    return;
  } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    throw std::runtime_error("failed to acquire swap chain image!");
  }

  vkResetFences(context.getDevice(), 1,
                &frameResource[currentFrame].inFlightFence);

  vkResetCommandBuffer(frameResource[currentFrame].commandBuffer,
                       /*VkCommandBufferResetFlagBits*/ 0);

  record(imageIndex);

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  VkPipelineStageFlags waitStages[] = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores =
      &frameResource[currentFrame].imageAvailableSemaphore;
  submitInfo.pWaitDstStageMask = waitStages;

  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &frameResource[currentFrame].commandBuffer;

  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores =
      &frameResource[currentFrame].renderFinishedSemaphore;

  if (vkQueueSubmit(context.graphicsQueue, 1, &submitInfo,
                    frameResource[currentFrame].inFlightFence) != VK_SUCCESS) {
    throw std::runtime_error("failed to submit draw command buffer!");
  }

  result = swapchain.present(
      imageIndex, frameResource[currentFrame].renderFinishedSemaphore);
  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
      GlfwContext::framebufferResized) {
    GlfwContext::framebufferResized = false;
    resizeWindow();
  } else if (result != VK_SUCCESS) {
    throw std::runtime_error("failed to present swap chain image!");
  }

  currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void PrtApplication::resizeWindow() {
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
  renderpass.destroyFramebuffers();
  renderpass.createFramebuffers(swapchain, colorImage, depthImage, w, h);
}

void PrtApplication::update() {
  glm::vec3 eye = glm::vec3(0.0f, 0.0f, 50.0f);
  float w = swapchain.width();
  float h = swapchain.height();
  glm::mat4 view = glm::lookAt(eye, glm::vec3(0.f), glm::vec3(0.f, 1.f, 0.f));
  glm::mat4 proj = glm::perspective(glm::radians(60.f), w / h, 0.1f, 1500.f);
  proj[1][1] *= -1;
  Transform trans;
  trans.transform = proj * view;
  frameResource[currentFrame].cameraTransform.upload(&trans, false);

  frameResource[currentFrame].lightCoef.upload(&lightCoef, false);
}

void PrtApplication::createDescriptorPool() {
  std::array<VkDescriptorPoolSize, 2> poolSizes{};
  poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  poolSizes[0].descriptorCount =
      static_cast<uint32_t>(4 * MAX_FRAMES_IN_FLIGHT);
  poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  poolSizes[1].descriptorCount =
      static_cast<uint32_t>(2 * MAX_FRAMES_IN_FLIGHT);

  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
  poolInfo.pPoolSizes = poolSizes.data();
  poolInfo.maxSets = static_cast<uint32_t>(3 * MAX_FRAMES_IN_FLIGHT);

  if (vkCreateDescriptorPool(context.device, &poolInfo, nullptr,
                             &descriptorPool) != VK_SUCCESS) {
    throw std::runtime_error("failed to create descriptor pool!");
  }
}

void PrtApplication::createSetLayoutModel() {
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
  if (vkCreateDescriptorSetLayout(context.device, &layoutInfo, nullptr,
                                  &setLayoutModel) != VK_SUCCESS) {
    throw std::runtime_error("failed to create descriptor set layout!");
  }
}

void PrtApplication::createSetLayoutObject() {
  std::array<VkDescriptorSetLayoutBinding, 2> bindings;
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

  VkDescriptorSetLayoutCreateInfo layoutInfo{};
  layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
  layoutInfo.pBindings = bindings.data();
  if (vkCreateDescriptorSetLayout(context.device, &layoutInfo, nullptr,
                                  &setLayoutObject) != VK_SUCCESS) {
    throw std::runtime_error("failed to create descriptor set layout!");
  }
}

void PrtApplication::createSetLayoutCubemap() {
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
  if (vkCreateDescriptorSetLayout(context.device, &layoutInfo, nullptr,
                                  &setLayoutCubemap) != VK_SUCCESS) {
    throw std::runtime_error("failed to create descriptor set layout!");
  }
}

void PrtApplication::createSyncObjects() {
  VkSemaphoreCreateInfo semaphoreInfo{};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fenceInfo{};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  for (size_t i = 0; i < frameResource.size(); i++) {
    if (vkCreateSemaphore(context.getDevice(), &semaphoreInfo, nullptr,
                          &frameResource[i].imageAvailableSemaphore) !=
            VK_SUCCESS ||
        vkCreateSemaphore(context.getDevice(), &semaphoreInfo, nullptr,
                          &frameResource[i].renderFinishedSemaphore) !=
            VK_SUCCESS ||
        vkCreateFence(context.getDevice(), &fenceInfo, nullptr,
                      &frameResource[i].inFlightFence) != VK_SUCCESS) {
      throw std::runtime_error(
          "failed to create synchronization objects for a frame!");
    }
  }
}

void PrtApplication::createCommandBuffers() {
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;
  allocInfo.commandPool = context.commandPool;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  std::array<VkCommandBuffer, MAX_FRAMES_IN_FLIGHT> handles;
  if (vkAllocateCommandBuffers(context.device, &allocInfo, handles.data()) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to allocate command buffers");
  }
  for (int i = 0; i < frameResource.size(); ++i) {
    frameResource[i].commandBuffer = handles[i];
  }
}

void PrtApplication::createDescriptorSets() {
  std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT,
                                             setLayoutObject);
  VkDescriptorSetAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = descriptorPool;
  allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
  allocInfo.pSetLayouts = layouts.data();

  std::vector<VkDescriptorSet> handles(MAX_FRAMES_IN_FLIGHT);
  if (vkAllocateDescriptorSets(context.device, &allocInfo, handles.data()) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to allocate descriptor sets!");
  }

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    frameResource[i].objectSet = handles[i];

    std::array<VkDescriptorBufferInfo, 2> bufferInfo;
    bufferInfo[0].buffer = frameResource[i].cameraTransform.handle;
    bufferInfo[0].offset = 0;
    bufferInfo[0].range = sizeof(Transform);
    bufferInfo[1].buffer = frameResource[i].lightCoef.handle;
    bufferInfo[1].offset = 0;
    bufferInfo[1].range = sizeof(LightCoef);

    std::array<VkWriteDescriptorSet, 2> descriptorWrites{};
    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet = frameResource[i].objectSet;
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].dstArrayElement = 0;
    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pBufferInfo = &bufferInfo[0];

    descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[1].dstSet = frameResource[i].objectSet;
    descriptorWrites[1].dstBinding = 1;
    descriptorWrites[1].dstArrayElement = 0;
    descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrites[1].descriptorCount = 1;
    descriptorWrites[1].pBufferInfo = &bufferInfo[1];

    vkUpdateDescriptorSets(context.device,
                           static_cast<uint32_t>(descriptorWrites.size()),
                           descriptorWrites.data(), 0, nullptr);
  }

  layouts.assign(MAX_FRAMES_IN_FLIGHT, setLayoutCubemap);
  allocInfo.pSetLayouts = layouts.data();

  if (vkAllocateDescriptorSets(context.device, &allocInfo, handles.data()) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to allocate descriptor sets!");
  }
  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    frameResource[i].cubemapSet = handles[i];

    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = cubemap.image.readView;
    imageInfo.sampler = cubemap.sampler;

    VkDescriptorBufferInfo bufferInfo;
    bufferInfo.buffer = frameResource[i].cameraTransform.handle;
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(Transform);

    std::array<VkWriteDescriptorSet, 2> descriptorWrites{};
    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet = frameResource[i].cubemapSet;
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].dstArrayElement = 0;
    descriptorWrites[0].descriptorType =
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pImageInfo = &imageInfo;

    descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[1].dstSet = frameResource[i].cubemapSet;
    descriptorWrites[1].dstBinding = 1;
    descriptorWrites[1].dstArrayElement = 0;
    descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrites[1].descriptorCount = 1;
    descriptorWrites[1].pBufferInfo = &bufferInfo;

    vkUpdateDescriptorSets(context.device,
                           static_cast<uint32_t>(descriptorWrites.size()),
                           descriptorWrites.data(), 0, nullptr);
  }
}

void PrtApplication::createFrameResource() {
  createSyncObjects();
  createCommandBuffers();
  for (int i = 0; i < frameResource.size(); ++i) {
    frameResource[i].cameraTransform.create(
        &context, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(Transform), true);
    frameResource[i].lightCoef.create(
        &context, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(LightCoef), true);
  }
  createDescriptorSets();
}

void PrtApplication::destroyFrameResource() {
  std::array<VkCommandBuffer, MAX_FRAMES_IN_FLIGHT> handles;
  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    handles[i] = frameResource[i].commandBuffer;
    vkDestroySemaphore(context.device, frameResource[i].imageAvailableSemaphore,
                       nullptr);
    vkDestroySemaphore(context.device, frameResource[i].renderFinishedSemaphore,
                       nullptr);
    vkDestroyFence(context.device, frameResource[i].inFlightFence, nullptr);

    frameResource[i].cameraTransform.destroy();
    frameResource[i].lightCoef.destroy();
  }
  vkFreeCommandBuffers(context.device, context.commandPool, handles.size(),
                       handles.data());
}

void PrtApplication::createRenderItems() {
  renderItem.resize(2);
  Transform model;
  model.transform = glm::scale(glm::mat4(1.0f), glm::vec3(100.0f));
  renderItem[0].indexCount = scene.meshes[0].indices.size();
  renderItem[0].indexOffset = scene.meshes[0].indexOffset;
  renderItem[0].vertexOffset = scene.meshes[0].vertexOffset;
  renderItem[0].model.create(&context, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                             sizeof(Transform), true);
  renderItem[0].model.upload(&model, false);

  model.transform = glm::scale(glm::mat4(1.0f), glm::vec3(100.0f));
  renderItem[1].indexCount = scene.meshes[1].indices.size();
  renderItem[1].indexOffset = scene.meshes[1].indexOffset;
  renderItem[1].vertexOffset = scene.meshes[1].vertexOffset;
  renderItem[1].model.create(&context, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                             sizeof(Transform), true);
  renderItem[1].model.upload(&model, false);

  std::vector<VkDescriptorSet> handles(renderItem.size(), VK_NULL_HANDLE);
  std::vector<VkDescriptorSetLayout> layouts(renderItem.size(), setLayoutModel);
  VkDescriptorSetAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = descriptorPool;
  allocInfo.descriptorSetCount = static_cast<uint32_t>(renderItem.size());
  allocInfo.pSetLayouts = layouts.data();
  if (vkAllocateDescriptorSets(context.device, &allocInfo, handles.data()) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to allocate descriptor sets!");
  }
  for (int i = 0; i < renderItem.size(); ++i) {
    renderItem[i].modelSet = handles[i];

    VkDescriptorBufferInfo bufferInfo;
    bufferInfo.buffer = renderItem[i].model.handle;
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(Transform);

    std::array<VkWriteDescriptorSet, 1> descriptorWrites{};
    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet = renderItem[i].modelSet;
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].dstArrayElement = 0;
    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pBufferInfo = &bufferInfo;

    vkUpdateDescriptorSets(context.device,
                           static_cast<uint32_t>(descriptorWrites.size()),
                           descriptorWrites.data(), 0, nullptr);
  }
}

void PrtApplication::destroyRenderItems() {
  std::vector<VkDescriptorSet> handles;
  for (RenderItem& item : renderItem) {
    item.model.destroy();
    handles.push_back(item.modelSet);
  }
}
