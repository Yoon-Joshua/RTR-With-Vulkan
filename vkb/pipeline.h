#pragma once
#include <vulkan/vulkan.h>

#include <vector>

#include "global.h"
#include "renderpass.h"

struct PipelineCreateInfo {
  VkVertexInputBindingDescription bindingDescription;
  std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
  VkSampleCountFlagBits sampleCount{VK_SAMPLE_COUNT_1_BIT};
  VkRenderPass renderPass;
  uint32_t subpass;
  std::string vs;
  std::string fs;
  std::vector<VkDescriptorSetLayout> setLayouts;
  VkCullModeFlagBits cull;
  uint32_t colorAttachmentNum = 1;
};

class Pipeline {
 public:
  VkPipeline handle;
  VkPipelineLayout layout;

  Context* context{nullptr};

  void defaultInit();

  void create(Context* cnt, PipelineCreateInfo& ci);

  void createPipeline(Context* context_, std::string vs, std::string fs,
                      std::vector<VkDescriptorSetLayout>& setLayouts,
                      RenderPass& pass);

  void createShadowPipeline(Context* context_,
                            std::vector<VkDescriptorSetLayout>& setLayouts,
                            RenderPass& pass);

  void createPipeline(Context* context_,
                      std::vector<VkDescriptorSetLayout>& setLayouts,
                      RenderPass& pass);

  void createPipelineNoTexture(Context* context_,
                               std::vector<VkDescriptorSetLayout>& setLayouts,
                               RenderPass& pass);

  /******** Assignment 2 PRT *********/
  void createPipelinePrt(Context* context_,
                         std::vector<VkDescriptorSetLayout>& setLayouts,
                         RenderPass& pass);

  /******** Assignment 2 GI *********/
  void createPipelineGBuffer(Context* context_,
                             std::vector<VkDescriptorSetLayout>& setLayouts,
                             RenderPass& pass);

  void createPipelineSSR(Context* context_,
                         std::vector<VkDescriptorSetLayout>& setLayouts,
                         RenderPass& pass);

  /******** Assignment 4 PBR *********/
  void createPipelineCubemap(Context* context_, std::string vs, std::string fs,
                             std::vector<VkDescriptorSetLayout>& setLayouts,
                             RenderPass& pass);

  void destroy();

 private:
  VkShaderModule createShaderModule(const std::vector<char>& code);
};