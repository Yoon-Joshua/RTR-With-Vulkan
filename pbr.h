#ifndef PBR_H
#define PBR_H

#include "global.h"
#include "application.h"
#include "vkb/common.h"
#include "vkb/texture.h"
#include "vkb/scene.h"
#include "vkb/renderpass.h"
#include "vkb/pipeline.h"

struct PbrPerFrame{
  VkDescriptorSet set;
  VkDescriptorSet skyboxset;
  Buffer cameraParam;
  Buffer transform;
  VkCommandBuffer commandBuffer;
  VkSemaphore imageAvailableSemaphore;
  VkSemaphore renderFinishedSemaphore;
  VkFence inFlightFence;
};

class PBRApplication : public Application {
 public:
  PBRApplication(GlfwContext*);
  void preCompute();
  void prepare();
  void cleanup();

 private:
  void record(uint32_t imageIndex);
  void update();
  void drawFrame();
  void resizeWindow();

  void createDescriptorPool();
  void createDescriptorSetLayout();

  void createSyncObjects();
  void createCommandBuffers();
  void createDescriptorSets();

  void createFrameResource();
  void destroyFrameResource();

  Scene scene;
  Image depthImage;
  Image colorImage;
  Texture energyAvg;
  Texture energy;
  Texture cubemap;

  RenderPass renderpass;
  Pipeline pipeline;
  Pipeline pipelineSkybox;

  VkDescriptorSetLayout setLayout{VK_NULL_HANDLE};
  VkDescriptorSetLayout skyboxSetLayout{VK_NULL_HANDLE};

  VkDescriptorPool descriptorPool{VK_NULL_HANDLE};

  PushConstant constant;
  Transform transform;
  CameraParam cameraParam;

  uint32_t currentFrame{0};
  std::array<PbrPerFrame, MAX_FRAMES_IN_FLIGHT> frameResource;

};

#endif