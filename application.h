#pragma once
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include "base/timer.h"
#include "global.h"
#include "vkb/swapchain.h"
#include "vkb/buffer.h"

struct Light {
  alignas(16) glm::vec3 pos;
  alignas(16) glm::vec3 intensity;
};

struct MVP {
  alignas(16) glm::mat4 model;
  alignas(16) glm::mat4 view;
  alignas(16) glm::mat4 proj;
};

struct CameraParam {
  alignas(16) glm::mat4 view;
  alignas(16) glm::mat4 proj;
  alignas(16) glm::vec3 eye;
};

struct Transform {
  alignas(16) glm::mat4 transform;
};

class RenderItem {
 public:
  uint32_t vertexOffset;
  uint32_t indexOffset;
  uint32_t indexCount;
  Buffer model;
  VkDescriptorSet modelSet;
};

class Application {
 public:
  Application(GlfwContext* glfwContext_);
  ~Application();
  virtual void prepare() = 0;
  virtual void mainLoop(Timer& timer);
  virtual void cleanup() = 0;

 protected:
  virtual void update() = 0;
  virtual void drawFrame() = 0;
  virtual void resizeWindow() = 0;

  GlfwContext* glfwContext{nullptr};
  Context context;
  SwapChain swapchain;

  VkFormat depthFormat{VK_FORMAT_UNDEFINED};
};