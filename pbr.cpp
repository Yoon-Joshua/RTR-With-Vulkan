#include "pbr.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <algorithm>
#include <random>

#include "stb_image.h"
#include "stb_image_write.h"
#include "vkb/buffer.h"

const float PI = 3.14159265358979323846f;

std::array<std::string, 6> box = {
    "asset/cubemap/CornellBox/posx.jpg", "asset/cubemap/CornellBox/negx.jpg",
    "asset/cubemap/CornellBox/posy.jpg", "asset/cubemap/CornellBox/negy.jpg",
    "asset/cubemap/CornellBox/posz.jpg", "asset/cubemap/CornellBox/negz.jpg"};

typedef struct samplePoints {
  std::vector<glm::vec3> directions;
  std::vector<float> PDFs;
} samplePoints;

samplePoints squareToCosineHemisphere(int sample_count) {
  samplePoints samlpeList;
  const int sample_side = static_cast<int>(floor(sqrt(sample_count)));

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<> rng(0.0, 1.0);
  for (int t = 0; t < sample_side; t++) {
    for (int p = 0; p < sample_side; p++) {
      double samplex = (t + rng(gen)) / sample_side;
      double sampley = (p + rng(gen)) / sample_side;

      double theta = 0.5f * acos(1 - 2 * samplex);
      double phi = 2 * PI * sampley;
      glm::vec3 wi =
          glm::vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
      float pdf = wi.z / PI;

      samlpeList.directions.push_back(wi);
      samlpeList.PDFs.push_back(pdf);
    }
  }
  return samlpeList;
}

glm::vec2 Hammersley(uint32_t i, uint32_t N) {  // 0-1
  uint32_t bits = (i << 16u) | (i >> 16u);
  bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
  bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
  bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
  bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
  float rdi = float(bits) * 2.3283064365386963e-10;
  return {float(i) / float(N), rdi};
}

PBRApplication::PBRApplication(GlfwContext* glfwCnt) : Application(glfwCnt) {}

void PBRApplication::preCompute() {
  const int resolution = 128;
  const int channel = 3;
  // const int sample_count = 1024;
  const int sample_count = 4096 * 4;
  float step = 1.0 / resolution;
  std::vector<uint8_t> data(resolution * resolution * 3);
  // origin --> left-bottom
  // i --> roughness --> height
  // j --> cos --> width

  glm::vec3 N(0.0, 0.0, 1.0);

#ifndef IMPORTANCE_SAMPLE
  std::function<float(glm::vec3, glm::vec3, float)> distributionGGX =
      [=](glm::vec3 N, glm::vec3 H, float roughness) -> float {
    float alpha = roughness * roughness;
    float NdotH = std::max(dot(N, H), 0.0f);
    return alpha * alpha /
           std::max(PI * powf(NdotH * NdotH * (alpha * alpha - 1) + 1, 2),
                    0.0001f);
  };

  std::function<float(float, float)> geometrySchlickGGX =
      [=](float NdotV, float roughness) -> float {
    // float k = (roughness * roughness) / 2.0f;
    // return NdotV / (NdotV * (1.0f - k) + k);
    float k = powf(roughness + 1, 2) / 8;
    return NdotV / (NdotV * (1.0f - k) + k);
  };

  std::function<float(float, float, float)> geometrySmith =
      [=](float roughness, float HdotV, float HdotL) -> float {
    return geometrySchlickGGX(HdotV, roughness) *
           geometrySchlickGGX(HdotL, roughness);
  };

  std::function<float(float, glm::vec3, glm::vec3)> brdf =
      [=](float roughness, glm::vec3 L, glm::vec3 V) -> float {
    L = glm::normalize(L);
    V = glm::normalize(V);
    float NdotL = std::max(glm::dot(L, N), 0.0f);
    float NdotV = std::max(glm::dot(N, V), 0.0f);

    glm::vec3 H = glm::normalize(L + V);
    float HdotL = std::max(glm::dot(L, H), 0.0f);
    float HdotV = std::max(glm::dot(H, V), 0.0f);

    float R0 = 1.0;
    float F = R0 + (1 - R0) * powf(1 - NdotL, 5);
    float D = distributionGGX(N, H, roughness);
    float G = geometrySmith(roughness, HdotV, HdotL);
    return F * D * G / 4 / NdotV / NdotL;
  };
#endif

  for (int i = 0; i < resolution; i++) {
    for (int j = 0; j < resolution; j++) {
#ifndef IMPORTANCE_SAMPLE
      float roughness = step * (static_cast<float>(i) + 0.5f);
      float NdotV = step * (static_cast<float>(j) + 0.5f);
      glm::vec3 V = glm::vec3(std::sqrt(1.f - NdotV * NdotV), 0.f, NdotV);

      samplePoints sampleList = squareToCosineHemisphere(sample_count);
      glm::vec3 irr = glm::vec3(0.0f, 0.0f, 0.0f);
      for (int k = 0; k < sample_count; k++) {
        glm::vec3 L = normalize(sampleList.directions[k]);
        float result = brdf(roughness, normalize(V), L) / sampleList.PDFs[k] *
                       std::max(glm::dot(N, L), 0.0f);
        irr.x += result;
        irr.y += result;
        irr.z += result;
      }
      irr /= sample_count;

      data[(i * resolution + j) * 3 + 0] = uint8_t(irr.x * 255.0);
      data[(i * resolution + j) * 3 + 1] = uint8_t(irr.y * 255.0);
      data[(i * resolution + j) * 3 + 2] = uint8_t(irr.z * 255.0);
#endif
    }
  }
  stbi_write_png("GGX_E_MC_LUT.png", resolution, resolution, 3, data.data(),
                 resolution * 3);
  std::cout << "Finished precomputed!" << std::endl;

  /************************* average energy **************************/
  std::vector<uint8_t> fuck(resolution * resolution * channel);

  for (int i = 0; i < resolution; i++) {
    float roughness = step * (static_cast<float>(i) + 0.5f);
    glm::vec3 e_avg = glm::vec3(0.0);
    for (int j = 0; j < resolution; j++) {
      float NdotV = step * (static_cast<float>(j) + 0.5f);
      glm::vec3 V = glm::vec3(std::sqrt(1.f - NdotV * NdotV), 0.f, NdotV);

      glm::vec3 ei = glm::vec3(data[3 * (resolution * i + j) + 0],
                               data[3 * (resolution * i + j) + 1],
                               data[3 * (resolution * i + j) + 2]);
      e_avg += 2.0f * ei * NdotV * step;
    }

    for (int j = 0; j < resolution; j++) {
      fuck[channel * (resolution * i + j) + 0] = static_cast<uint8_t>(e_avg.x);
      fuck[channel * (resolution * i + j) + 1] = static_cast<uint8_t>(e_avg.y);
      fuck[channel * (resolution * i + j) + 2] = static_cast<uint8_t>(e_avg.z);
    }
  }
  stbi_write_png("GGX_Eavg_LUT.png", resolution, resolution, 3, fuck.data(),
                 resolution * 3);
  std::cout << "Finished precomputed!" << std::endl;
}

void PBRApplication::prepare() {
  // preCompute();

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

  cubemap.create(&context, box);
  energyAvg.create(&context, "asset/GGX_Eavg_LUT.png");
  energy.create(&context, "asset/GGX_E_MC_LUT.png");

  renderpass.create(&context, swapchain.getFormat(), depthFormat,
                    context.msaaSamples);
  renderpass.createFramebuffers(swapchain, colorImage, depthImage,
                                swapchain.width(), swapchain.height());

  createDescriptorPool();
  createDescriptorSetLayout();
  std::vector<VkDescriptorSetLayout> layouts = {setLayout};
  pipeline.createPipeline(&context, "shader/pbr/object.vert.spv",
                          "shader/pbr/object.frag.spv", layouts, renderpass);
  layouts = {skyboxSetLayout};
  pipelineSkybox.createPipelineCubemap(&context, "shader/pbr/skybox.vert.spv",
                                      "shader/pbr/skybox.frag.spv", layouts,
                                      renderpass);
  scene.loadObj("asset/ball/ball.obj");
  scene.loadObj("asset/cube/cube.obj");
  scene.createGeometryBuffer(&context);

  createFrameResource();
}

void PBRApplication::cleanup() {
  destroyFrameResource();
  depthImage.destroy();
  colorImage.destroy();
  energyAvg.destroy();
  energy.destroy();
  cubemap.destroy();
  pipeline.destroy();
  pipelineSkybox.destroy();
  renderpass.destroyFramebuffers();
  renderpass.destroy();
  scene.cleanup();
  vkDestroyDescriptorPool(context.device, descriptorPool, nullptr);
  vkDestroyDescriptorSetLayout(context.device, setLayout, nullptr);
  vkDestroyDescriptorSetLayout(context.device, skyboxSetLayout, nullptr);
}

void PBRApplication::record(uint32_t imageIndex) {
  PbrPerFrame& fr = frameResource[currentFrame];

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

  if (vkBeginCommandBuffer(fr.commandBuffer, &beginInfo) != VK_SUCCESS) {
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
  clearValues[0].color = {{1.0f, 1.0f, 1.0f, 1.0f}};
  clearValues[1].depthStencil = {1.0f, 0};
  renderPassInfo.clearValueCount = clearValues.size();
  renderPassInfo.pClearValues = clearValues.data();
  vkCmdBeginRenderPass(fr.commandBuffer, &renderPassInfo,
                       VK_SUBPASS_CONTENTS_INLINE);
  VkBuffer vertexBuffers[] = {scene.vertexBuffer.handle};
  VkDeviceSize offsets[] = {0};
  vkCmdBindVertexBuffers(fr.commandBuffer, 0, 1, vertexBuffers, offsets);
  vkCmdBindIndexBuffer(fr.commandBuffer, scene.indexBuffer.handle, 0,
                       VK_INDEX_TYPE_UINT32);
  VkViewport viewport{};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = swapchain.width();
  viewport.height = swapchain.height();
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  vkCmdSetViewport(fr.commandBuffer, 0, 1, &viewport);
  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent.width = swapchain.width();
  scissor.extent.height = swapchain.height();
  vkCmdSetScissor(fr.commandBuffer, 0, 1, &scissor);
  uint32_t firstIndex = static_cast<uint32_t>(scene.meshes[1].indexOffset);
  uint32_t indexCount = static_cast<uint32_t>(scene.meshes[1].indices.size());
  uint32_t vertexOffset = static_cast<uint32_t>(scene.meshes[1].vertexOffset);
  vkCmdBindPipeline(fr.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipelineSkybox.handle);
  vkCmdBindDescriptorSets(fr.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          pipelineSkybox.layout, 0, 1,
                          &frameResource[currentFrame].skyboxset, 0, nullptr);

  vkCmdDrawIndexed(fr.commandBuffer, indexCount, 1, firstIndex, vertexOffset,
                   0);
  vkCmdBindPipeline(fr.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipeline.handle);
  vkCmdBindDescriptorSets(fr.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          pipeline.layout, 0, 1,
                          &frameResource[currentFrame].set, 0, nullptr);

  firstIndex = static_cast<uint32_t>(scene.meshes[0].indexOffset);
  indexCount = static_cast<uint32_t>(scene.meshes[0].indices.size());
  vertexOffset = static_cast<uint32_t>(scene.meshes[0].vertexOffset);

  int num = 2;
  float interval = 1.0f / (2 * num + 2);
  for (int i = -num; i <= num; ++i) {
    constant.model =
        glm::translate(glm::mat4(1.0f), glm::vec3(15.f * i, 10.f, 0.f));
    constant.roughness[0] = interval * (1 + i + num);
    constant.roughness[1] = 1;
    vkCmdPushConstants(
        fr.commandBuffer, pipeline.layout,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
        sizeof(PushConstant), &constant);
    vkCmdDrawIndexed(fr.commandBuffer, indexCount, 1, firstIndex, vertexOffset,
                     0);
  }
  for (int i = -num; i <= num; ++i) {
    constant.model =
        glm::translate(glm::mat4(1.0f), glm::vec3(15.f * i, -10.f, 0.f));
    constant.roughness[0] = interval * (1 + i + num);
    constant.roughness[1] = 0;
    vkCmdPushConstants(
        fr.commandBuffer, pipeline.layout,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
        sizeof(PushConstant), &constant);
    vkCmdDrawIndexed(fr.commandBuffer, indexCount, 1, firstIndex, vertexOffset,
                     0);
  }
  vkCmdEndRenderPass(fr.commandBuffer);

  if (vkEndCommandBuffer(fr.commandBuffer) != VK_SUCCESS) {
    throw std::runtime_error("failed to record command buffer!");
  }
}

void PBRApplication::update() {
  float width = swapchain.width();
  float height = swapchain.height();
  cameraParam.eye = glm::vec3(0.f, 10.f, 50.f);
  cameraParam.view = glm::lookAt(cameraParam.eye, glm::vec3(0.f, 0.f, 0.f),
                                 glm::vec3(0.f, 1.f, 0.f));
  cameraParam.proj =
      glm::perspective(glm::radians(60.f), width / height, 0.1f, 1000.f);
  cameraParam.proj[1][1] *= -1;
  frameResource[currentFrame].cameraParam.upload(&cameraParam, false);

  transform.transform =
      cameraParam.proj * cameraParam.view *
      glm::scale(glm::mat4(1.f), glm::vec3(200.f, 200.f, 200.f));
  frameResource[currentFrame].transform.upload(&transform, false);
}

void PBRApplication::drawFrame() {
  PbrPerFrame& fr = frameResource[currentFrame];

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

void PBRApplication::resizeWindow() {
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

void PBRApplication::createDescriptorPool() {
  std::array<VkDescriptorPoolSize, 2> poolSizes{};
  poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  poolSizes[0].descriptorCount =
      static_cast<uint32_t>(2 * MAX_FRAMES_IN_FLIGHT);
  poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  poolSizes[1].descriptorCount =
      static_cast<uint32_t>(3 * MAX_FRAMES_IN_FLIGHT);

  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
  poolInfo.pPoolSizes = poolSizes.data();
  poolInfo.maxSets = static_cast<uint32_t>(2 * MAX_FRAMES_IN_FLIGHT);

  if (vkCreateDescriptorPool(context.device, &poolInfo, nullptr,
                             &descriptorPool) != VK_SUCCESS) {
    throw std::runtime_error("failed to create descriptor pool!");
  }
}

void PBRApplication::createDescriptorSetLayout() {
  std::vector<VkDescriptorSetLayoutBinding> bindings(3);
  bindings[0].binding = 0;
  bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  bindings[0].descriptorCount = 1;
  bindings[0].stageFlags =
      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
  bindings[0].pImmutableSamplers = nullptr;  // Optional
  bindings[1].binding = 1;
  bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  bindings[1].descriptorCount = 1;
  bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  bindings[1].pImmutableSamplers = nullptr;
  bindings[2].binding = 2;
  bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  bindings[2].descriptorCount = 1;
  bindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  bindings[2].pImmutableSamplers = nullptr;

  VkDescriptorSetLayoutCreateInfo layoutInfo{};
  layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
  layoutInfo.pBindings = bindings.data();
  if (vkCreateDescriptorSetLayout(context.device, &layoutInfo, nullptr,
                                  &setLayout) != VK_SUCCESS) {
    throw std::runtime_error("failed to create descriptor set layout!");
  }

  bindings.resize(2);
  bindings[0].binding = 0;
  bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  bindings[0].descriptorCount = 1;
  bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  bindings[0].pImmutableSamplers = nullptr;
  bindings[1].binding = 1;
  bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  bindings[1].descriptorCount = 1;
  bindings[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  bindings[1].pImmutableSamplers = nullptr;  // Optional

  layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
  layoutInfo.pBindings = bindings.data();
  if (vkCreateDescriptorSetLayout(context.device, &layoutInfo, nullptr,
                                  &skyboxSetLayout) != VK_SUCCESS) {
    throw std::runtime_error("failed to create descriptor set layout!");
  }
}

void PBRApplication::createSyncObjects() {
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

void PBRApplication::createDescriptorSets() {
  std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> set;
  std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, setLayout);
  VkDescriptorSetAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = descriptorPool;
  allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
  allocInfo.pSetLayouts = layouts.data();
  if (vkAllocateDescriptorSets(context.device, &allocInfo, set.data()) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to allocate descriptor sets!");
  }
  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    frameResource[i].set = set[i];

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = frameResource[i].cameraParam.handle;
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(CameraParam);
    VkDescriptorImageInfo energyAvgInfo{};
    energyAvgInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    energyAvgInfo.imageView = energyAvg.image.readView;
    energyAvgInfo.sampler = energyAvg.sampler;
    VkDescriptorImageInfo energyInfo{};
    energyInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    energyInfo.imageView = energy.image.readView;
    energyInfo.sampler = energy.sampler;
    std::array<VkWriteDescriptorSet, 3> write{};
    write[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write[0].dstSet = frameResource[i].set;
    write[0].dstBinding = 0;
    write[0].dstArrayElement = 0;
    write[0].descriptorCount = 1;
    write[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write[0].pBufferInfo = &bufferInfo;
    write[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write[1].dstSet = frameResource[i].set;
    write[1].dstBinding = 1;
    write[1].dstArrayElement = 0;
    write[1].descriptorCount = 1;
    write[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write[1].pImageInfo = &energyAvgInfo;
    write[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write[2].dstSet = frameResource[i].set;
    write[2].dstBinding = 2;
    write[2].dstArrayElement = 0;
    write[2].descriptorCount = 1;
    write[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write[2].pImageInfo = &energyInfo;
    vkUpdateDescriptorSets(context.device, static_cast<uint32_t>(write.size()),
                           write.data(), 0, nullptr);
  }

  layouts.assign(MAX_FRAMES_IN_FLIGHT, skyboxSetLayout);
  allocInfo.pSetLayouts = layouts.data();
  if (vkAllocateDescriptorSets(context.device, &allocInfo, set.data()) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to allocate descriptor sets!");
  }
  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    frameResource[i].skyboxset = set[i];

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = cubemap.image.readView;
    imageInfo.sampler = cubemap.sampler;
    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = frameResource[i].transform.handle;
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(Transform);
    std::array<VkWriteDescriptorSet, 2> write{};
    write[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write[0].dstSet = frameResource[i].skyboxset;
    write[0].dstBinding = 0;
    write[0].dstArrayElement = 0;
    write[0].descriptorCount = 1;
    write[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write[0].pImageInfo = &imageInfo;
    write[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write[1].dstSet = frameResource[i].skyboxset;
    write[1].dstBinding = 1;
    write[1].dstArrayElement = 0;
    write[1].descriptorCount = 1;
    write[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write[1].pBufferInfo = &bufferInfo;
    vkUpdateDescriptorSets(context.device, static_cast<uint32_t>(write.size()),
                           write.data(), 0, nullptr);
  }
}

void PBRApplication::createCommandBuffers() {
  std::array<VkCommandBuffer, MAX_FRAMES_IN_FLIGHT> handles;
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = context.commandPool;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = handles.size();
  if (vkAllocateCommandBuffers(context.device, &allocInfo, handles.data())){
    throw std::runtime_error("failed to allocate command buffers");
  }
  for (int i = 0; i < handles.size(); ++i) {
    frameResource[i].commandBuffer = handles[i];
  }
}

void PBRApplication::createFrameResource() {
  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    frameResource[i].cameraParam.create(&context,
                                        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                        sizeof(CameraParam), true);
    frameResource[i].transform.create(
        &context, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(Transform), true);
  }
  createSyncObjects();
  createDescriptorSets();
  createCommandBuffers();
}

void PBRApplication::destroyFrameResource() {
  std::array<VkCommandBuffer, MAX_FRAMES_IN_FLIGHT> handles;
  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    auto& t = frameResource[i];
    t.cameraParam.destroy();
    t.transform.destroy();
    vkDestroySemaphore(context.device, t.imageAvailableSemaphore, nullptr);
    vkDestroySemaphore(context.device, t.renderFinishedSemaphore, nullptr);
    vkDestroyFence(context.device, t.inFlightFence, nullptr);
    handles[i] = t.commandBuffer;
  }
  vkFreeCommandBuffers(context.device, context.commandPool,
                       MAX_FRAMES_IN_FLIGHT, handles.data());
}