#ifndef GI_H
#define GI_H

#include "application.h"
#include "vkb/pipeline.h"
#include "vkb/renderpass.h"
#include "vkb/scene.h"
#include "vkb/texture.h"

struct SsrPerFrame {
  VkCommandBuffer commandBuffer{VK_NULL_HANDLE};
  VkSemaphore imageAvailableSemaphore{VK_NULL_HANDLE};
  VkSemaphore renderFinishedSemaphore{VK_NULL_HANDLE};
  VkFence inFlightFence{VK_NULL_HANDLE};

  Buffer lightTrans;
  Buffer cameraParam;
  Buffer light;
  VkDescriptorSet lightTransSet{VK_NULL_HANDLE};
  VkDescriptorSet cubeSet{VK_NULL_HANDLE};
  VkDescriptorSet floorSet{VK_NULL_HANDLE};
  VkDescriptorSet renderSet{VK_NULL_HANDLE};
}; 

class SsrApplication : public Application {
 public:
  SsrApplication(GlfwContext* glfwContext);
  void prepare();
  void cleanup();

 private:
  void record(uint32_t imageIndex);
  void drawFrame();
  void resizeWindow();
  void update();

  void createDescriptorPool();
  void createDescriptorSetLayout();
  VkDescriptorSetLayout transformSetLayout{VK_NULL_HANDLE};
  VkDescriptorSetLayout cubeSetLayout{VK_NULL_HANDLE};
  VkDescriptorSetLayout floorSetLayout{VK_NULL_HANDLE};
  VkDescriptorSetLayout renderSetLayout{VK_NULL_HANDLE};

  Scene scene;
  Image depthImage;
  Image colorImage;
  Texture texture;

  // G-Buffers
  Texture shadowMap;
  Texture normalMap;
  Texture worldPosMap;
  Texture depthMap;
  Texture colorMap;

  Pipeline pipelineShadow;
  Pipeline pipelineCube;
  Pipeline pipelineFloor;
  Pipeline pipelineSsr;

  uint32_t currentFrame = 0;

  RenderPass gbufferPass;
  RenderPass shadowPass;
  RenderPass ssrPass;

  VkDescriptorPool descriptorPool{VK_NULL_HANDLE};

  CameraParam cameraParam;
  Light light;
  std::array<SsrPerFrame, MAX_FRAMES_IN_FLIGHT> frameResource;
  void createSyncObjects();
  void createCommandBuffers();
  void createDescriptorSets();
  void createFrameResources();
  void destroyFrameResources();

  std::vector<RenderItem> renderItem;
  void createRenderItem();
  void destroyRenderItem();
};

#endif