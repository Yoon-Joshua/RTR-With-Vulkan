#ifndef PCSS_H
#define PCSS_H

#include "application.h"
#include "vkb/pipeline.h"
#include "vkb/scene.h"
#include "vkb/texture.h"

struct PcssPerFrame {
  VkSemaphore imageAvailableSemaphore;
  VkSemaphore renderFinishedSemaphore;
  VkFence inFlightFence;
  /// @brief for shadow mapping
  Buffer transform;
  /// @brief for drawing
  Buffer cameraParam;
  Buffer light;
  VkCommandBuffer commandBuffer;
  VkDescriptorSet phongSet;
  VkDescriptorSet textureSet;
  VkDescriptorSet shadowSet;
};

/// @note Swapchain and Surface are necessary.
class PCSSApplication : public Application {
 public:
  PCSSApplication(GlfwContext*);
  void prepare();
  void cleanup();

 private:
  void record(uint32_t imageIndex);
  void drawFrame();
  void resizeWindow();
  void update();
  /************************************************/
  void createDescriptorPool();

  void createSetLayoutCommon();
  void createSetLayoutTexture();
  void createSetLayoutShadow();
  /************************************************/
  Scene scene;
  Image depthImage;
  Image colorImage;
  Texture texture;
  Texture shadowmap;

  /// @name Pipeline
  /// @{
  Pipeline pipeline;
  Pipeline pipelineNoTexture;
  Pipeline pipelineShadow;
  /// @}

  uint32_t currentFrame = 0;

  RenderPass renderpass;
  RenderPass shadowpass;
  VkDescriptorPool descriptorPool{VK_NULL_HANDLE};

  VkDescriptorSetLayout setLayoutPhong;
  VkDescriptorSetLayout setLayoutTexture;
  VkDescriptorSetLayout setLayoutTransform;

  Transform transform;
  CameraParam cameraParam;
  Light lightInfo;

  std::array<PcssPerFrame, MAX_FRAMES_IN_FLIGHT> frameResource;
  void createSyncObjects();
  void createCommandBuffers();
  void createDescriptorSets();
  void createFrameResource();
  void destroyFrameResource();

  std::vector<RenderItem> renderItem;
  void createRenderItems();
  void destroyRenderItems();
};

#endif