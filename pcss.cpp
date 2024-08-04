#include "pcss.h"

#include <chrono>

#include "global.h"
#include "vkb/common.h"

/*
shadow map的pass需要一个vp矩阵和各个物体的model矩阵

第二个pass需要各个物体的model矩阵、相机的vp矩阵、光源的vp矩阵、物体的贴图
*/

const std::string TEXTURE_PATH = "asset/viking_room/viking_room.png";

PCSSApplication::PCSSApplication(GlfwContext* glfwCnt) : Application(glfwCnt) {}

void PCSSApplication::prepare() {
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

  shadowmap.create(
      &context, SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT, VK_FORMAT_D16_UNORM,
      VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
      VK_IMAGE_ASPECT_DEPTH_BIT);

  renderpass.create(&context, swapchain.getFormat(), depthFormat,
                    context.msaaSamples);
  renderpass.createFramebuffers(swapchain, colorImage, depthImage,
                                swapchain.width(), swapchain.height());

  shadowpass.create(&context, VK_FORMAT_D16_UNORM);
  shadowpass.createFramebuffers(shadowmap.image, SHADOW_MAP_WIDTH,
                                SHADOW_MAP_HEIGHT);

  texture.create(&context, TEXTURE_PATH.c_str());

  createDescriptorPool();
  createSetLayoutCommon();
  createSetLayoutTexture();
  createSetLayoutShadow();

  std::vector<VkDescriptorSetLayout> layouts;
  layouts = {setLayoutTransform, setLayoutPhong, setLayoutTexture};
  pipeline.createPipeline(&context, layouts, renderpass);
  layouts = {setLayoutTransform, setLayoutPhong};
  pipelineNoTexture.createPipelineNoTexture(&context, layouts, renderpass);
  layouts = {setLayoutTransform, setLayoutTransform};
  pipelineShadow.createShadowPipeline(&context, layouts, shadowpass);

  scene.loadObj("asset/plane/plane.obj");
  scene.loadObj("asset/viking_room/viking_room.obj");
  scene.createGeometryBuffer(&context);
  createRenderItems();

  createFrameResource();
}

void PCSSApplication::cleanup() {
  destroyFrameResource();
  destroyRenderItems();
  renderpass.destroyFramebuffers();
  shadowpass.destroyFramebuffers();
  colorImage.destroy();
  depthImage.destroy();
  texture.destroy();
  shadowmap.destroy();
  renderpass.destroy();
  shadowpass.destroy();
  pipeline.destroy();
  pipelineNoTexture.destroy();
  pipelineShadow.destroy();

  vkDestroyDescriptorSetLayout(context.device, setLayoutPhong, nullptr);
  vkDestroyDescriptorSetLayout(context.device, setLayoutTexture, nullptr);
  vkDestroyDescriptorSetLayout(context.device, setLayoutTransform, nullptr);
  vkDestroyDescriptorPool(context.device, descriptorPool, nullptr);

  scene.cleanup();
}

void PCSSApplication::record(uint32_t imageIndex) {
  VkCommandBuffer& cb = frameResource[currentFrame].commandBuffer;

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
  renderPassInfo.renderArea.offset = {0, 0};
  renderPassInfo.renderArea.extent.width = SHADOW_MAP_WIDTH;
  renderPassInfo.renderArea.extent.height = SHADOW_MAP_HEIGHT;
  std::array<VkClearValue, 2> clearValues{};
  clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
  clearValues[1].depthStencil = {1.0f, 0};
  renderPassInfo.clearValueCount = 1;
  renderPassInfo.pClearValues = &clearValues[1];
  vkCmdBeginRenderPass(cb, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

  vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineShadow.handle);
  VkBuffer vertexBuffers[] = {scene.vertexBuffer.handle};
  VkDeviceSize offsets[] = {0};
  vkCmdBindVertexBuffers(cb, 0, 1, vertexBuffers, offsets);
  vkCmdBindIndexBuffer(cb, scene.indexBuffer.handle, 0, VK_INDEX_TYPE_UINT32);
  vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          pipelineShadow.layout, 1, 1,
                          &frameResource[currentFrame].shadowSet, 0, nullptr);

  VkViewport viewport{};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = SHADOW_MAP_WIDTH;
  viewport.height = SHADOW_MAP_HEIGHT;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  vkCmdSetViewport(cb, 0, 1, &viewport);
  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent.width = SHADOW_MAP_WIDTH;
  scissor.extent.height = SHADOW_MAP_HEIGHT;
  vkCmdSetScissor(cb, 0, 1, &scissor);
  for (RenderItem& item : renderItem) {
    vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipelineShadow.layout, 0, 1, &item.modelSet, 0,
                            nullptr);
    vkCmdDrawIndexed(cb, item.indexCount, 1, item.indexOffset,
                     item.vertexOffset, 0);
  }
  vkCmdEndRenderPass(cb);

  // second pass
  clearValues[1].depthStencil = {1.0f, 0};
  renderPassInfo.renderPass = renderpass.handle;
  renderPassInfo.framebuffer = renderpass.getFramebuffer(imageIndex);
  renderPassInfo.renderArea.extent.width = swapchain.width();
  renderPassInfo.renderArea.extent.height = swapchain.height();
  renderPassInfo.clearValueCount = 2;
  renderPassInfo.pClearValues = clearValues.data();
  vkCmdBeginRenderPass(cb, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
  vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.handle);
  std::array<VkDescriptorSet, 2> temp = {
      frameResource[currentFrame].phongSet,
      frameResource[currentFrame].textureSet};
  vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.layout,
                          1, 2, temp.data(), 0, nullptr);
  viewport.width = swapchain.width();
  viewport.height = swapchain.height();
  vkCmdSetViewport(cb, 0, 1, &viewport);
  scissor.extent.width = swapchain.width();
  scissor.extent.height = swapchain.height();
  vkCmdSetScissor(cb, 0, 1, &scissor);
  for (int i = 1; i < 3; ++i) {
    RenderItem& item = renderItem[i];
    vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipeline.layout, 0, 1, &item.modelSet, 0,
                            nullptr);
    vkCmdDrawIndexed(cb, item.indexCount, 1, item.indexOffset,
                     item.vertexOffset, 0);
  }
  RenderItem& item = renderItem[0];
  temp = {item.modelSet, frameResource[currentFrame].phongSet};
  vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipelineNoTexture.handle);
  vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          pipelineNoTexture.layout, 0, 2, temp.data(), 0,
                          nullptr);
  vkCmdDrawIndexed(cb, item.indexCount, 1, item.indexOffset, item.vertexOffset,
                   0);
  vkCmdEndRenderPass(cb);

  if (vkEndCommandBuffer(cb) != VK_SUCCESS) {
    throw std::runtime_error("failed to record command buffer!");
  }
}

void PCSSApplication::drawFrame() {
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

void PCSSApplication::resizeWindow() {
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

void PCSSApplication::update() {
  float w = swapchain.width();
  float h = swapchain.height();

  lightInfo.pos = glm::vec3(0.0, 80.0, 80.0);
  lightInfo.intensity = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
  frameResource[currentFrame].light.upload(&lightInfo, false);

  cameraParam.eye = glm::vec3(80.0f,80.f,0);
  cameraParam.view = glm::lookAt(cameraParam.eye, glm::vec3(0.0f),
                                 glm::vec3(0.0f, 1.0f, 0.0f));
  cameraParam.proj = glm::perspective(glm::radians(60.f), w / h, 0.1f, 1000.0f);
  cameraParam.proj[1][1] *= -1;
  frameResource[currentFrame].cameraParam.upload(&cameraParam, false);

  transform.transform = glm::lookAt(lightInfo.pos, glm::vec3(0.0, 0.0, 0.0),
                                    glm::vec3(0.0, 1.0, 0.0));
  glm::mat4 proj = glm::ortho(-150.0f, 150.0f, -150.0f, 150.0f, 50.0f, 300.0f);
  proj[1][1] *= -1;
  transform.transform = proj * transform.transform;
  frameResource[currentFrame].transform.upload(&transform, false);
}

void PCSSApplication::createDescriptorPool() {
  std::array<VkDescriptorPoolSize, 2> poolSizes{};
  poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  poolSizes[0].descriptorCount =
      static_cast<uint32_t>(12 * MAX_FRAMES_IN_FLIGHT);
  poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  poolSizes[1].descriptorCount =
      static_cast<uint32_t>(4 * MAX_FRAMES_IN_FLIGHT);

  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
  poolInfo.pPoolSizes = poolSizes.data();
  poolInfo.maxSets = static_cast<uint32_t>(7 * MAX_FRAMES_IN_FLIGHT);

  if (vkCreateDescriptorPool(context.device, &poolInfo, nullptr,
                             &descriptorPool) != VK_SUCCESS) {
    throw std::runtime_error("failed to create descriptor pool!");
  }
}

void PCSSApplication::createSetLayoutCommon() {
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
  bindings[2].stageFlags =
      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
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
  if (vkCreateDescriptorSetLayout(context.device, &layoutInfo, nullptr,
                                  &setLayoutPhong) != VK_SUCCESS) {
    throw std::runtime_error("failed to create descriptor set layout!");
  }
}

void PCSSApplication::createSetLayoutTexture() {
  std::array<VkDescriptorSetLayoutBinding, 1> bindings;
  bindings[0].binding = 0;
  bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  bindings[0].descriptorCount = 1;
  bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  bindings[0].pImmutableSamplers = nullptr;  // Optional

  VkDescriptorSetLayoutCreateInfo layoutInfo{};
  layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
  layoutInfo.pBindings = bindings.data();
  if (vkCreateDescriptorSetLayout(context.device, &layoutInfo, nullptr,
                                  &setLayoutTexture) != VK_SUCCESS) {
    throw std::runtime_error("failed to create descriptor set layout!");
  }
}

void PCSSApplication::createSetLayoutShadow() {
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
                                  &setLayoutTransform) != VK_SUCCESS) {
    throw std::runtime_error("failed to create descriptor set layout!");
  }
}

void PCSSApplication::createSyncObjects() {
  VkSemaphoreCreateInfo semaphoreInfo{};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fenceInfo{};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    if (vkCreateSemaphore(context.device, &semaphoreInfo, nullptr,
                          &frameResource[i].imageAvailableSemaphore) !=
            VK_SUCCESS ||
        vkCreateSemaphore(context.device, &semaphoreInfo, nullptr,
                          &frameResource[i].renderFinishedSemaphore) !=
            VK_SUCCESS ||
        vkCreateFence(context.device, &fenceInfo, nullptr,
                      &frameResource[i].inFlightFence) != VK_SUCCESS) {
      throw std::runtime_error(
          "failed to create synchronization objects for a frame!");
    }
  }
}

void PCSSApplication::createCommandBuffers() {
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

void PCSSApplication::createDescriptorSets() {
  std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT,
                                             setLayoutPhong);
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
    frameResource[i].phongSet = handles[i];

    std::vector<VkDescriptorBufferInfo> bufferInfo(4);
    bufferInfo[0].buffer = frameResource[i].cameraParam.handle;
    bufferInfo[0].offset = 0;
    bufferInfo[0].range = sizeof(CameraParam);
    bufferInfo[1].buffer = frameResource[i].transform.handle;
    bufferInfo[1].offset = 0;
    bufferInfo[1].range = sizeof(Transform);
    bufferInfo[2].buffer = frameResource[i].light.handle;
    bufferInfo[2].offset = 0;
    bufferInfo[2].range = sizeof(Light);

    VkDescriptorImageInfo imageInfo{};
    imageInfo.sampler = shadowmap.sampler;
    imageInfo.imageView = shadowmap.image.readView;
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    std::array<VkWriteDescriptorSet, 4> descriptorWrites{};
    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet = frameResource[i].phongSet;
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].dstArrayElement = 0;
    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pBufferInfo = &bufferInfo[0];

    descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[1].dstSet = frameResource[i].phongSet;
    descriptorWrites[1].dstBinding = 1;
    descriptorWrites[1].dstArrayElement = 0;
    descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrites[1].descriptorCount = 1;
    descriptorWrites[1].pBufferInfo = &bufferInfo[1];

    descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[2].dstSet = frameResource[i].phongSet;
    descriptorWrites[2].dstBinding = 2;
    descriptorWrites[2].dstArrayElement = 0;
    descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrites[2].descriptorCount = 1;
    descriptorWrites[2].pBufferInfo = &bufferInfo[2];

    descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[3].dstSet = frameResource[i].phongSet;
    descriptorWrites[3].dstBinding = 3;
    descriptorWrites[3].dstArrayElement = 0;
    descriptorWrites[3].descriptorType =
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[3].descriptorCount = 1;
    descriptorWrites[3].pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(context.device,
                           static_cast<uint32_t>(descriptorWrites.size()),
                           descriptorWrites.data(), 0, nullptr);
  }

  layouts.assign(MAX_FRAMES_IN_FLIGHT, setLayoutTexture);
  allocInfo.pSetLayouts = layouts.data();
  if (vkAllocateDescriptorSets(context.device, &allocInfo, handles.data()) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to allocate descriptor sets!");
  }
  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    frameResource[i].textureSet = handles[i];

    VkDescriptorImageInfo imageInfo{};
    imageInfo.sampler = texture.sampler;
    imageInfo.imageView = texture.image.readView;
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    std::array<VkWriteDescriptorSet, 1> descriptorWrites{};
    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet = frameResource[i].textureSet;
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].dstArrayElement = 0;
    descriptorWrites[0].descriptorType =
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pImageInfo = &imageInfo;
    vkUpdateDescriptorSets(context.device,
                           static_cast<uint32_t>(descriptorWrites.size()),
                           descriptorWrites.data(), 0, nullptr);
  }

  layouts.assign(MAX_FRAMES_IN_FLIGHT, setLayoutTransform);
  allocInfo.pSetLayouts = layouts.data();
  if (vkAllocateDescriptorSets(context.device, &allocInfo, handles.data()) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to allocate descriptor sets!");
  }
  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    frameResource[i].shadowSet = handles[i];

    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = frameResource[i].transform.handle;
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(Transform);

    std::array<VkWriteDescriptorSet, 1> descriptorWrites{};
    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet = frameResource[i].shadowSet;
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].dstArrayElement = 0;
    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pBufferInfo = &bufferInfo;
    vkUpdateDescriptorSets(context.device,
                           static_cast<uint32_t>(descriptorWrites.size()),
                           descriptorWrites.data(), 0, nullptr);
  }

  layouts.assign(renderItem.size(), setLayoutTransform);
  allocInfo.pSetLayouts = layouts.data();
  allocInfo.descriptorSetCount = layouts.size();
  handles.resize(renderItem.size());
  if (vkAllocateDescriptorSets(context.device, &allocInfo, handles.data()) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to allocate descriptor sets!");
  }
  for (size_t i = 0; i < renderItem.size(); i++) {
    renderItem[i].modelSet = handles[i];

    VkDescriptorBufferInfo bufferInfo{};
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

void PCSSApplication::createFrameResource() {
  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    frameResource[i].transform.create(
        &context, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(Transform), true);
    frameResource[i].cameraParam.create(&context,
                                        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                        sizeof(CameraParam), true);
    frameResource[i].light.create(&context, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                  sizeof(Light), true);
  }
  createSyncObjects();
  createCommandBuffers();
  createDescriptorSets();
}

void PCSSApplication::destroyFrameResource() {
  std::array<VkCommandBuffer, MAX_FRAMES_IN_FLIGHT> handles;
  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    handles[i] = frameResource[i].commandBuffer;
    vkDestroySemaphore(context.device, frameResource[i].imageAvailableSemaphore,
                       nullptr);
    vkDestroySemaphore(context.device, frameResource[i].renderFinishedSemaphore,
                       nullptr);
    vkDestroyFence(context.device, frameResource[i].inFlightFence, nullptr);

    frameResource[i].cameraParam.destroy();
    frameResource[i].transform.destroy();
    frameResource[i].light.destroy();
  }
  vkFreeCommandBuffers(context.device, context.commandPool, handles.size(),
                       handles.data());
}

void PCSSApplication::createRenderItems() {
  renderItem.resize(3);

  // floor
  glm::mat4 m =
      glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -30.0f)),
                 glm::vec3(8.0f * 27.924055f));
  renderItem[0].indexCount = scene.meshes[0].indices.size();
  renderItem[0].indexOffset = scene.meshes[0].indexOffset;
  renderItem[0].vertexOffset = scene.meshes[0].vertexOffset;
  renderItem[0].model.create(&context, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                             sizeof(glm::mat4), true);
  renderItem[0].model.upload(&m, false);

  // big marry
  m = glm::scale(glm::mat4(1.0f), glm::vec3(20.0f, 20.0f, 20.0f));
  renderItem[1].indexCount = scene.meshes[1].indices.size();
  renderItem[1].indexOffset = scene.meshes[1].indexOffset;
  renderItem[1].vertexOffset = scene.meshes[1].vertexOffset;
  renderItem[1].model.create(&context, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                             sizeof(glm::mat4), true);
  renderItem[1].model.upload(&m, false);

  // small marry
  // 滚一边去
  m = glm::scale(
      glm::translate(glm::mat4(1.0f), glm::vec3(400.0f, 0.0f, -400.0f)),
      glm::vec3(10.0f, 10.0f, 10.0f));
  renderItem[2].indexCount = scene.meshes[1].indices.size();
  renderItem[2].indexOffset = scene.meshes[1].indexOffset;
  renderItem[2].vertexOffset = scene.meshes[1].vertexOffset;
  renderItem[2].model.create(&context, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                             sizeof(glm::mat4), true);
  renderItem[2].model.upload(&m, false);
}

void PCSSApplication::destroyRenderItems() {
  for (RenderItem& item : renderItem) {
    item.model.destroy();
  }
}
