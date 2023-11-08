#ifndef PRT_H
#define PRT_H

#include "application.h"
#include "vkb/pipeline.h"
#include "vkb/scene.h"
#include "vkb/texture.h"

struct LightCoef {
  alignas(16) glm::mat3 r;
  alignas(16) glm::mat3 g;
  alignas(16) glm::mat3 b;
};

struct PrtPerFrame {
  VkDescriptorSet objectSet;
  VkDescriptorSet cubemapSet;
  Buffer cameraTransform;
  /// @remark 环境光的光照系数一般是不变的，但是也可以利用球谐函数的性质使环境旋转。
  Buffer lightCoef;
  VkCommandBuffer commandBuffer;
  VkSemaphore imageAvailableSemaphore;
  VkSemaphore renderFinishedSemaphore;
  VkFence inFlightFence;
};

/// @note Swapchain and Surface are necessary.
class PrtApplication : public Application {
 public:
  PrtApplication(GlfwContext*);
  void prepare() override;
  void cleanup() override;

 private:
  void record(uint32_t imageIndex);
  void drawFrame() override;
  void resizeWindow();
  void update() override;

  void createDescriptorPool();
  void createSetLayoutModel();
  void createSetLayoutObject();
  void createSetLayoutCubemap();
  
  LightCoef lightCoef;
  Scene scene;
  Image depthImage;
  Image colorImage;
  Texture cubemap;

  Pipeline objectPipeline;
  Pipeline cubemapPipeline;

  VkDescriptorSetLayout setLayoutModel{VK_NULL_HANDLE};
  VkDescriptorSetLayout setLayoutObject{VK_NULL_HANDLE};
  VkDescriptorSetLayout setLayoutCubemap{VK_NULL_HANDLE};

  uint32_t currentFrame = 0;

  RenderPass renderpass;
  VkDescriptorPool descriptorPool{VK_NULL_HANDLE};

  std::array<PrtPerFrame, MAX_FRAMES_IN_FLIGHT> frameResource;
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