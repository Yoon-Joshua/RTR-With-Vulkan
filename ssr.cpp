#include "ssr.h"

const std::string TEXTURE_PATH = "asset/cube/checker.png";

SsrApplication::SsrApplication(GlfwContext *glfwContext)
    : Application(glfwContext) {}

void SsrApplication::prepare() {
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

  shadowMap.create(
      &context, SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT, VK_FORMAT_D16_UNORM,
      VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
      VK_IMAGE_ASPECT_DEPTH_BIT);
  normalMap.create(
      &context, SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT, VK_FORMAT_R8G8B8A8_SNORM,
      VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
      VK_IMAGE_ASPECT_COLOR_BIT);
  worldPosMap.create(
      &context, SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT,
      VK_FORMAT_R32G32B32A32_SFLOAT,
      VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
      VK_IMAGE_ASPECT_COLOR_BIT);
  depthMap.create(
      &context, SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT, depthFormat,
      VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
      VK_IMAGE_ASPECT_DEPTH_BIT);
  colorMap.create(
      &context, SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT, VK_FORMAT_R8G8B8A8_UNORM,
      VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
      VK_IMAGE_ASPECT_COLOR_BIT);
  texture.create(&context, TEXTURE_PATH);

  shadowPass.create(&context, VK_FORMAT_D16_UNORM);
  shadowPass.createFramebuffers(shadowMap.image, SHADOW_MAP_WIDTH,
                                SHADOW_MAP_HEIGHT);
  gbufferPass.create(&context, VK_FORMAT_R32G32B32A32_SFLOAT,
                     VK_FORMAT_R8G8B8A8_SNORM, VK_FORMAT_R8G8B8A8_UNORM,
                     depthFormat);
  gbufferPass.createFramebuffers(colorMap.image, worldPosMap.image,
                                 normalMap.image, depthMap.image,
                                 SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT);
  ssrPass.create(&context, swapchain.getFormat(), depthFormat,
                 context.msaaSamples);
  ssrPass.createFramebuffers(swapchain, colorImage, depthImage,
                             swapchain.width(), swapchain.height());

  createDescriptorSetLayout();
  PipelineCreateInfo info{};
  info.attributeDescriptions = Vertex::getAttributeDescriptions();
  info.bindingDescription = Vertex::getBindingDescription();
  info.cull = VK_CULL_MODE_BACK_BIT;
  info.vs = "shader/gi/shadow.vert.spv";
  info.fs = "shader/gi/shadow.frag.spv";
  info.renderPass = shadowPass.handle;
  info.subpass = 0;
  info.sampleCount = VK_SAMPLE_COUNT_1_BIT;
  info.setLayouts = {transformSetLayout, transformSetLayout};
  pipelineShadow.create(&context, info);

  info.vs = "shader/gi/object.vert.spv";
  info.fs = "shader/gi/object.frag.spv";
  info.renderPass = gbufferPass.handle;
  info.setLayouts = {transformSetLayout, cubeSetLayout};
  info.colorAttachmentNum = 3;
  pipelineCube.create(&context, info);

  info.vs = "shader/gi/floor.vert.spv";
  info.fs = "shader/gi/floor.frag.spv";
  info.setLayouts = {transformSetLayout, floorSetLayout};
  pipelineFloor.create(&context, info);

  info.vs = "shader/gi/ssr.vert.spv";
  info.fs = "shader/gi/ssr.frag.spv";
  info.setLayouts = {transformSetLayout, renderSetLayout};
  info.renderPass = ssrPass.handle;
  info.sampleCount = context.msaaSamples;
  info.colorAttachmentNum = 1;
  pipelineSsr.create(&context, info);

  createDescriptorPool();

  scene.loadGLFT("asset/cube/cube2.gltf");
  scene.createGeometryBuffer(&context);

  createRenderItem();
  createFrameResources();
}

void SsrApplication::cleanup() {
  destroyFrameResources();
  destroyRenderItem();

  scene.cleanup();
  vkDestroyDescriptorPool(context.device, descriptorPool, nullptr);

  pipelineCube.destroy();
  pipelineShadow.destroy();
  pipelineFloor.destroy();
  pipelineSsr.destroy();

  shadowPass.destroyFramebuffers();
  vkDestroyRenderPass(context.device, shadowPass.handle, nullptr);
  gbufferPass.destroyFramebuffers();
  vkDestroyRenderPass(context.device, gbufferPass.handle, nullptr);
  ssrPass.destroyFramebuffers();
  vkDestroyRenderPass(context.device, ssrPass.handle, nullptr);

  vkDestroyDescriptorSetLayout(context.device, transformSetLayout, nullptr);
  vkDestroyDescriptorSetLayout(context.device, cubeSetLayout, nullptr);
  vkDestroyDescriptorSetLayout(context.device, floorSetLayout, nullptr);
  vkDestroyDescriptorSetLayout(context.device, renderSetLayout, nullptr);

  shadowMap.destroy();
  normalMap.destroy();
  worldPosMap.destroy();
  depthMap.destroy();
  colorMap.destroy();
  texture.destroy();

  depthImage.destroy();
  colorImage.destroy();
}

void SsrApplication::record(uint32_t imageIndex) {
  VkCommandBuffer &cb = frameResource[currentFrame].commandBuffer;

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
  renderPassInfo.renderPass = shadowPass.handle;
  renderPassInfo.framebuffer = shadowPass.getFramebuffer(0);
  renderPassInfo.renderArea.offset = {0, 0};
  renderPassInfo.renderArea.extent.width = SHADOW_MAP_WIDTH;
  renderPassInfo.renderArea.extent.height = SHADOW_MAP_HEIGHT;
  VkClearValue clearDepth{};
  clearDepth.depthStencil = {1.0f, 0};
  renderPassInfo.clearValueCount = 1;
  renderPassInfo.pClearValues = &clearDepth;
  vkCmdBeginRenderPass(cb, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
  VkBuffer vertexBuffers[] = {scene.vertexBuffer.handle};
  VkDeviceSize offsets[] = {0};
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
  scissor.offset = {0, 0};
  scissor.extent.width = SHADOW_MAP_WIDTH;
  scissor.extent.height = SHADOW_MAP_HEIGHT;
  vkCmdSetScissor(cb, 0, 1, &scissor);
  vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineShadow.handle);
  vkCmdBindDescriptorSets(
      cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineShadow.layout, 1, 1,
      &frameResource[currentFrame].lightTransSet, 0, nullptr);
  for (RenderItem &item : renderItem) {
    vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipelineShadow.layout, 0, 1, &item.modelSet, 0,
                            nullptr);
    vkCmdDrawIndexed(cb, item.indexCount, 1, item.indexOffset,
                     item.vertexOffset, 0);
  }
  vkCmdEndRenderPass(cb);

  // the pass to generate G-Buffer
  renderPassInfo.renderPass = gbufferPass.handle;
  renderPassInfo.framebuffer = gbufferPass.getFramebuffer(0);
  // renderPassInfo.framebuffer = gbufferPass.getFramebuffer(imageIndex);
  std::array<VkClearValue, 4> clearValues{};
  clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
  clearValues[1].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
  clearValues[2].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
  clearValues[3].depthStencil = {1.0f, 0};
  renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
  renderPassInfo.pClearValues = clearValues.data();
  vkCmdBeginRenderPass(cb, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

  vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineCube.handle);
  std::vector<VkDescriptorSet> sets = {renderItem[0].modelSet,
                                       frameResource[currentFrame].cubeSet};
  vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          pipelineCube.layout, 0, sets.size(), sets.data(), 0,
                          nullptr);
  vkCmdDrawIndexed(cb, renderItem[0].indexCount, 1, renderItem[0].indexOffset,
                   renderItem[0].vertexOffset, 0);
  vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineFloor.handle);
  sets = {renderItem[1].modelSet, frameResource[currentFrame].floorSet};
  vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          pipelineFloor.layout, 0, sets.size(), sets.data(), 0,
                          nullptr);
  vkCmdDrawIndexed(cb, renderItem[1].indexCount, 1, renderItem[1].indexOffset,
                   renderItem[1].vertexOffset, 0);
  vkCmdEndRenderPass(cb);

  // the pass to render the final image
  viewport.width = w;
  viewport.height = h;
  vkCmdSetViewport(cb, 0, 1, &viewport);
  scissor.extent.width = w;
  scissor.extent.height = h;
  vkCmdSetScissor(cb, 0, 1, &scissor);
  renderPassInfo.renderArea.extent.width = w;
  renderPassInfo.renderArea.extent.height = h;

  renderPassInfo.renderPass = ssrPass.handle;
  renderPassInfo.framebuffer = ssrPass.getFramebuffer(imageIndex);
  std::array<VkClearValue, 2> clearValuesSSR{};
  clearValuesSSR[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
  clearValuesSSR[1].depthStencil = {1.0f, 0};
  renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValuesSSR.size());
  renderPassInfo.pClearValues = clearValuesSSR.data();
  vkCmdBeginRenderPass(cb, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
  vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineSsr.handle);
  vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          pipelineSsr.layout, 1, 1,
                          &frameResource[currentFrame].renderSet, 0, nullptr);
  for (RenderItem &item : renderItem) {
    vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipelineSsr.layout, 0, 1, &item.modelSet, 0,
                            nullptr);
    vkCmdDrawIndexed(cb, item.indexCount, 1, item.indexOffset,
                     item.vertexOffset, 0);
  }
  vkCmdEndRenderPass(cb);

  if (vkEndCommandBuffer(cb) != VK_SUCCESS) {
    throw std::runtime_error("failed to record command buffer!");
  }
}

void SsrApplication::drawFrame() {
  SsrPerFrame &fr = frameResource[currentFrame];

  vkWaitForFences(context.device, 1, &fr.inFlightFence, VK_TRUE, UINT64_MAX);

  uint32_t imageIndex;
  VkResult result =
      swapchain.acquireNextImage(fr.imageAvailableSemaphore, imageIndex);
  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    resizeWindow();
    return;
  } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    throw std::runtime_error("failed to acquire swap chain image!");
  }

  vkResetFences(context.device, 1, &fr.inFlightFence);
  vkResetCommandBuffer(fr.commandBuffer,
                       /*VkCommandBufferResetFlagBits*/ 0);
  record(imageIndex);

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  VkPipelineStageFlags waitStages[] = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = &fr.imageAvailableSemaphore;
  submitInfo.pWaitDstStageMask = waitStages;

  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &fr.commandBuffer;

  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = &fr.renderFinishedSemaphore;

  if (vkQueueSubmit(context.graphicsQueue, 1, &submitInfo, fr.inFlightFence) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to submit draw command buffer!");
  }

  result = swapchain.present(imageIndex, fr.renderFinishedSemaphore);
  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
      GlfwContext::framebufferResized) {
    GlfwContext::framebufferResized = false;
    resizeWindow();
  } else if (result != VK_SUCCESS) {
    throw std::runtime_error("failed to present swap chain image!");
  }

  currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void SsrApplication::resizeWindow() {
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

  ssrPass.destroyFramebuffers();
  ssrPass.createFramebuffers(swapchain, colorImage, depthImage, w, h);
}

void SsrApplication::update() {
  glm::vec3 eye = {-5.f, 2.0f, 5.f};
  glm::vec3 target = {0.0f, 0.0f, 0.0f};
  cameraParam.view = glm::lookAt(eye, target, glm::vec3(0.f, 1.f, 0.f));
  cameraParam.proj = glm::perspective(
      glm::radians(65.f), (float)swapchain.width() / (float)swapchain.height(),
      0.1f, 100.f);
  cameraParam.proj[1][1] *= -1;
  cameraParam.eye = eye;
  frameResource[currentFrame].cameraParam.upload(&cameraParam, false);

  light.pos = glm::vec3(-13.45f, 13.40507f, 0);
  light.intensity = glm::vec3(1.0f, 1.0f, 1.0f);
  frameResource[currentFrame].light.upload(&light, false);

  glm::mat4 view =
      glm::lookAt(light.pos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
  glm::mat4 proj = glm::ortho(-30.0f, 30.0f, -30.0f, 30.f, 10.f, 50.0f);
  proj[1][1] *= -1;
  Transform trans = {proj * view};
  frameResource[currentFrame].lightTrans.upload(&trans, false);
}

void SsrApplication::createDescriptorPool() {
  std::array<VkDescriptorPoolSize, 2> poolSizes{};
  poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  poolSizes[0].descriptorCount =
      static_cast<uint32_t>(10 * MAX_FRAMES_IN_FLIGHT);
  poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  poolSizes[1].descriptorCount =
      static_cast<uint32_t>(10 * MAX_FRAMES_IN_FLIGHT);

  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
  poolInfo.pPoolSizes = poolSizes.data();
  poolInfo.maxSets = static_cast<uint32_t>(10 * MAX_FRAMES_IN_FLIGHT);

  if (vkCreateDescriptorPool(context.device, &poolInfo, nullptr,
                             &descriptorPool) != VK_SUCCESS) {
    throw std::runtime_error("failed to create descriptor pool!");
  }
}

void SsrApplication::createDescriptorSetLayout() {
  std::vector<VkDescriptorSetLayoutBinding> bindings(1);
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
                                  &transformSetLayout) != VK_SUCCESS) {
    throw std::runtime_error("failed to create descriptor set layout!");
  }

  bindings.resize(2);
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
  layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
  layoutInfo.pBindings = bindings.data();
  if (vkCreateDescriptorSetLayout(context.device, &layoutInfo, nullptr,
                                  &cubeSetLayout) != VK_SUCCESS) {
    throw std::runtime_error("failed to create descriptor set layout!");
  }

  bindings.resize(1);
  bindings[0].binding = 0;
  bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  bindings[0].descriptorCount = 1;
  bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  bindings[0].pImmutableSamplers = nullptr;  // Optional

  layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
  layoutInfo.pBindings = bindings.data();
  if (vkCreateDescriptorSetLayout(context.device, &layoutInfo, nullptr,
                                  &floorSetLayout) != VK_SUCCESS) {
    throw std::runtime_error("failed to create descriptor set layout!");
  }

  bindings.resize(4);
  bindings[0].binding = 0;
  bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  bindings[0].descriptorCount = 1;
  bindings[0].stageFlags =
      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
  bindings[0].pImmutableSamplers = nullptr;  // Optional
  bindings[1].binding = 1;
  bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  bindings[1].descriptorCount = 1;
  bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  bindings[1].pImmutableSamplers = nullptr;  // Optional

  bindings[2].binding = 2;
  bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  bindings[2].descriptorCount = 1;
  bindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  bindings[2].pImmutableSamplers = nullptr;  // Optional

  bindings[3].binding = 3;
  bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  bindings[3].descriptorCount = 5;
  bindings[3].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  bindings[3].pImmutableSamplers = nullptr;  // Optional

  layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
  layoutInfo.pBindings = bindings.data();
  if (vkCreateDescriptorSetLayout(context.device, &layoutInfo, nullptr,
                                  &renderSetLayout) != VK_SUCCESS) {
    throw std::runtime_error("failed to create descriptor set layout!");
  }
}

void SsrApplication::createSyncObjects() {
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

void SsrApplication::createCommandBuffers() {
  std::array<VkCommandBuffer, MAX_FRAMES_IN_FLIGHT> handles;
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = context.commandPool;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = handles.size();
  if (vkAllocateCommandBuffers(context.device, &allocInfo, handles.data())) {
    throw std::runtime_error("failed to allocate command buffers");
  }
  for (int i = 0; i < handles.size(); ++i) {
    frameResource[i].commandBuffer = handles[i];
  }
}

void SsrApplication::createDescriptorSets() {
  std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT,
                                             transformSetLayout);
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
    frameResource[i].lightTransSet = handles[i];

    VkDescriptorBufferInfo bufferInfo;
    bufferInfo.buffer = frameResource[i].lightTrans.handle;
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(Transform);

    std::array<VkWriteDescriptorSet, 1> descriptorWrites{};
    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet = frameResource[i].lightTransSet;
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].dstArrayElement = 0;
    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pBufferInfo = &bufferInfo;
    vkUpdateDescriptorSets(context.device,
                           static_cast<uint32_t>(descriptorWrites.size()),
                           descriptorWrites.data(), 0, nullptr);
  }
  /**************************************************************************/
  layouts.assign(MAX_FRAMES_IN_FLIGHT, cubeSetLayout);
  allocInfo.pSetLayouts = layouts.data();
  if (vkAllocateDescriptorSets(context.device, &allocInfo, handles.data()) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to allocate descriptor sets!");
  }

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    frameResource[i].cubeSet = handles[i];

    VkDescriptorBufferInfo bufferInfo;
    bufferInfo.buffer = frameResource[i].cameraParam.handle;
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(CameraParam);

    VkDescriptorImageInfo imageInfo{};
    imageInfo.sampler = texture.sampler;
    imageInfo.imageView = texture.image.readView;
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    std::array<VkWriteDescriptorSet, 2> descriptorWrites{};
    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet = frameResource[i].cubeSet;
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].dstArrayElement = 0;
    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pBufferInfo = &bufferInfo;
    descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[1].dstSet = frameResource[i].cubeSet;
    descriptorWrites[1].dstBinding = 1;
    descriptorWrites[1].dstArrayElement = 0;
    descriptorWrites[1].descriptorType =
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[1].descriptorCount = 1;
    descriptorWrites[1].pImageInfo = &imageInfo;
    vkUpdateDescriptorSets(context.device,
                           static_cast<uint32_t>(descriptorWrites.size()),
                           descriptorWrites.data(), 0, nullptr);
  }
  /**************************************************************************/
  layouts.assign(MAX_FRAMES_IN_FLIGHT, floorSetLayout);
  allocInfo.pSetLayouts = layouts.data();
  if (vkAllocateDescriptorSets(context.device, &allocInfo, handles.data()) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to allocate descriptor sets!");
  }

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    frameResource[i].floorSet = handles[i];

    VkDescriptorBufferInfo bufferInfo;
    bufferInfo.buffer = frameResource[i].cameraParam.handle;
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(CameraParam);

    std::array<VkWriteDescriptorSet, 1> descriptorWrites{};
    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet = frameResource[i].floorSet;
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].dstArrayElement = 0;
    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pBufferInfo = &bufferInfo;
    vkUpdateDescriptorSets(context.device,
                           static_cast<uint32_t>(descriptorWrites.size()),
                           descriptorWrites.data(), 0, nullptr);
  }
  /**************************************************************************/
  layouts.assign(MAX_FRAMES_IN_FLIGHT, renderSetLayout);
  allocInfo.pSetLayouts = layouts.data();
  if (vkAllocateDescriptorSets(context.device, &allocInfo, handles.data()) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to allocate descriptor sets!");
  }

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    frameResource[i].renderSet = handles[i];

    VkDescriptorBufferInfo cameraBufferInfo;
    cameraBufferInfo.buffer = frameResource[i].cameraParam.handle;
    cameraBufferInfo.offset = 0;
    cameraBufferInfo.range = sizeof(CameraParam);

    VkDescriptorBufferInfo lightBufferInfo;
    lightBufferInfo.buffer = frameResource[i].light.handle;
    lightBufferInfo.offset = 0;
    lightBufferInfo.range = sizeof(Light);

    VkDescriptorBufferInfo lightTransBufferInfo{};
    lightTransBufferInfo.buffer = frameResource[i].lightTrans.handle;
    lightTransBufferInfo.offset = 0;
    lightTransBufferInfo.range = sizeof(Transform);

    std::array<VkDescriptorImageInfo, 5> imageInfo = {
        shadowMap.sampler,
        shadowMap.image.readView,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        normalMap.sampler,
        normalMap.image.readView,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        depthMap.sampler,
        depthMap.image.readView,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        colorMap.sampler,
        colorMap.image.readView,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        worldPosMap.sampler,
        worldPosMap.image.readView,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };

    std::array<VkWriteDescriptorSet, 4> descriptorWrites{};
    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet = frameResource[i].renderSet;
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].dstArrayElement = 0;
    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pBufferInfo = &cameraBufferInfo;
    descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[1].dstSet = frameResource[i].renderSet;
    descriptorWrites[1].dstBinding = 1;
    descriptorWrites[1].dstArrayElement = 0;
    descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrites[1].descriptorCount = 1;
    descriptorWrites[1].pBufferInfo = &lightBufferInfo;
    descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[2].dstSet = frameResource[i].renderSet;
    descriptorWrites[2].dstBinding = 2;
    descriptorWrites[2].dstArrayElement = 0;
    descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrites[2].descriptorCount = 1;
    descriptorWrites[2].pBufferInfo = &lightTransBufferInfo;
    descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[3].dstSet = frameResource[i].renderSet;
    descriptorWrites[3].dstBinding = 3;
    descriptorWrites[3].dstArrayElement = 0;
    descriptorWrites[3].descriptorType =
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[3].descriptorCount = imageInfo.size();
    descriptorWrites[3].pImageInfo = imageInfo.data();
    vkUpdateDescriptorSets(context.device,
                           static_cast<uint32_t>(descriptorWrites.size()),
                           descriptorWrites.data(), 0, nullptr);
  }
}

void SsrApplication::createFrameResources() {
  createCommandBuffers();
  createSyncObjects();
  for (auto &f : frameResource) {
    f.cameraParam.create(&context, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                         sizeof(CameraParam), true);
    f.lightTrans.create(&context, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                        sizeof(Transform), true);
    f.light.create(&context, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(Light),
                   true);
  }
  createDescriptorSets();
}

void SsrApplication::destroyFrameResources() {
  for (auto &f : frameResource) {
    vkDestroySemaphore(context.device, f.imageAvailableSemaphore, nullptr);
    vkDestroySemaphore(context.device, f.renderFinishedSemaphore, nullptr);
    vkDestroyFence(context.device, f.inFlightFence, nullptr);
    f.cameraParam.destroy();
    f.lightTrans.destroy();
    f.light.destroy();
  }
}

void SsrApplication::createRenderItem() {
  renderItem.resize(2);
  Transform model;
  model.transform = glm::mat4(1.0f);
  renderItem[0].indexCount = scene.meshes[0].indices.size();
  renderItem[0].indexOffset = scene.meshes[0].indexOffset;
  renderItem[0].vertexOffset = scene.meshes[0].vertexOffset;
  renderItem[0].model.create(&context, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                             sizeof(Transform), true);
  renderItem[0].model.upload(&model, false);

  renderItem[1].indexCount = scene.meshes[1].indices.size();
  renderItem[1].indexOffset = scene.meshes[1].indexOffset;
  renderItem[1].vertexOffset = scene.meshes[1].vertexOffset;
  renderItem[1].model.create(&context, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                             sizeof(Transform), true);
  renderItem[1].model.upload(&model, false);

  std::vector<VkDescriptorSet> handles(renderItem.size(), VK_NULL_HANDLE);
  std::vector<VkDescriptorSetLayout> layouts(renderItem.size(),
                                             transformSetLayout);
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

void SsrApplication::destroyRenderItem() {
  std::vector<VkDescriptorSet> handles;
  for (RenderItem &item : renderItem) {
    item.model.destroy();
    handles.push_back(item.modelSet);
  }
}